/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
// #include "mqtt_client.h"
#include "device_wifi.h"
#include "network_mqtt_task.h"
#include "network_http.h"
#include "tuya_sdk.h"
#include "cJSON.h"
#include "driver/uart.h"
#define UART_MAX_NUM_RX_BYTES (255+4)

static const char *TAG = "MAIN";
#define BUF_SIZE (1024)

int user_ceil(float num) {
    int inum = (int)num;
    if (num == (float)inum) {
        return inum;
    }
    return inum + 1;
}





// ------------------------- cmd_id -------------------------
// 0x00 双方通用的应答包 数据长度 1 数据区 0xff nack
// 0x01 esp询问stm32的版本信息                  nack
// 0x02 stm32给esp发送版本信息                  nack
// 0x03 esp发送给stm32 进入bool模式             ack
// 0x04 stm32发送esp 已经处于bool模式           nack
// 0x05 esp将固件拆分成N/128分发送到stm32       ack
// ------------------------- cmd_id -------------------------

// 串口帧头
#define UART_DATA_HEAD_1 0xED
#define UART_DATA_HEAD_2 0x90
// 串口帧头
// 是否需要应答 发送方发送ack 接收方需要回传应答包 否则重新发送
// static char uart_isack = 0;
#define UART_DATA_ACK_3  0X01 
#define UART_DATA_NACK_3 0X02
// 是否需要应答

// 数据长度
// static char UART_DATA_LENGTH = 0;
// 数据长度

// 数据区
// 固件分包 第几个分包
// static char uart_firmware_count = 0;
// 固件分包 第几个分包
// ....
// 数据区

// 校验和 整条数据做校验
// static char uart_data_sum_high = 0;
// static char uart_data_sum_low  = 0;
// 校验和 整条数据做校验


// 计算 CRCCRC-16/MODBUS
// crc modbus 校验和遵循 低位在前 高位在后
// https://blog.csdn.net/m0_37697335/article/details/113267780
uint16_t calculateCRC(char *data, int length) {
    unsigned int i;
    unsigned short crc = 0xFFFF;  //crc16位寄存器初始值
    // printf("crc16_modbus校验->length:%d data:",length);
    while(length--)
    {
        // printf("%02x ",*data);
        crc ^= *data++;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xA001; //多项式 POLY（0x8005)的高低位交换值，这是由于其模型的一些参数决定的
            }
            else
            {
                crc = (crc >> 1);
            }
        }
    }
    // printf("\r\n");
    return crc;
}
// 等待ack的信号
SemaphoreHandle_t uart_ota_wait_ack_semap = NULL;

// uart_data 传入一个空指针进来
// cmd_id 命令id
// isack 是否应答
// data 数据区
// data_length 数据区的长度
int uart_send_cmdid_data_handle(char **uart_rx_data, uint8_t cmd_id, uint8_t isack, char* data, int data_length)
{
    
    int uart_rx_data_length = data_length + 8;
    *uart_rx_data = malloc(uart_rx_data_length * sizeof(char));

    (*uart_rx_data)[0] = UART_DATA_HEAD_1;
    (*uart_rx_data)[1] = UART_DATA_HEAD_2;
    (*uart_rx_data)[2] = cmd_id;
    (*uart_rx_data)[3] = isack;

    (*uart_rx_data)[4] =  (data_length >> 8) & 0xFF; // 数据长度 高位
    (*uart_rx_data)[5] = data_length & 0xFF;       // 数据长度 低位

    char *uart_data_ = &((*uart_rx_data)[6]);
    memcpy(uart_data_, data, data_length * sizeof(char));

    // -2 是要把校验位剪了
    uint16_t crc = calculateCRC(*uart_rx_data, uart_rx_data_length - 2);
    (*uart_rx_data)[uart_rx_data_length - 2] = crc & 0xFF;       // 校验和 低位
    (*uart_rx_data)[uart_rx_data_length - 1] = (crc >> 8) & 0xFF; // 校验和 高位

    return uart_rx_data_length;
}
// 发送ack命令
void uart_send_ack_data(void)
{
    char *uart_rx_data = NULL;
    static char data[] = {255};
    int uart_rx_data_length = uart_send_cmdid_data_handle(&uart_rx_data, 0x00, 0x00, data, 1);
    uart_write_bytes(UART_NUM_0, uart_rx_data, uart_rx_data_length);
    // 输出固件debug
    printf("串口命令debug:");
    for(int i = 0; i < uart_rx_data_length; i++){
        printf("%02x ",((char*)uart_rx_data)[i]);
    }
    printf("\r\n");
    free(uart_rx_data);
    // free(data);
}

// 开始分批发送固件包
esp_err_t ota_send_firmware(tuya_ota_info *tuya_otoInfo,http_files_data *hf_data)
{
    // 固件拆分次数
    char* firmware = (char*)hf_data->data;
    int firmware_size = hf_data->readData_count;
    memset(firmware, 0x66, firmware_size);
    int firmware_split_number = (int)user_ceil((double)((double)firmware_size / 128.0000));

    char *uart_rx_data = NULL;
    int uart_rx_data_length = 0;
    for (int i = 0; i < firmware_split_number; i++)
    {
       	int firmware_block_size = 0;	// 固件包大小
		ESP_LOGI("ota_uart:","第%d次拆分->固件当前索引位:%d", i, (i * 128));
		if(firmware_size - (i*128) > 128 )
		{
			firmware_block_size = 128;
		}
		else{
			firmware_block_size = firmware_size - ((i)*128);
		}
		printf("分包大小：%d\r\n",firmware_block_size);
        char *firmware_block = malloc(1 + firmware_block_size);// +1 是因为需要加上现在是第几个固件包 
        firmware_block[0] = i;  // 现在是第i个固件包
        char *temp = &firmware_block[1];
        memcpy( temp, (char *)(firmware+(i * 128)) ,  firmware_block_size * sizeof(char));
        uart_rx_data_length = uart_send_cmdid_data_handle(&uart_rx_data, 0x03, 0x01, firmware_block, firmware_block_size + 1);
        uart_write_bytes(UART_NUM_0, uart_rx_data, uart_rx_data_length);
      
        // 输出固件debug
        printf("串口命令debug:");
		for(int i = 0; i < uart_rx_data_length; i++){
			printf("%02x ",((char*)uart_rx_data)[i]);
		}
		printf("\r\n");
	    // 输出固件debug

        ESP_LOGI("ota_uart:","等待ack中");
        if(xSemaphoreTake(uart_ota_wait_ack_semap, 1000) == pdTRUE)
        {
            ESP_LOGE("ota_uart:","发送%d固件包成功",i);
        }
        else{
            ESP_LOGE("ota_uart:","发送固件包失败:接收方未返回应答信号");
            uint8_t anew_count = 0;  // 重新发送五次
            bool success_flag = false;
            while(anew_count < 5){
                ESP_LOGE("ota_uart:","重发机制启动,重新发送%d固件包",i);
                anew_count++;
                // 再次等待 五次后就抛出异常了
                uart_write_bytes(UART_NUM_0, uart_rx_data, uart_rx_data_length);
                if(xSemaphoreTake(uart_ota_wait_ack_semap, 1000) == pdTRUE)
                {
                    ESP_LOGE("ota_uart:","发送%d固件包成功",i);
                    success_flag = true;  // 设置成功标志
                    break;  // 跳出循环，因为成功发送
                }  
                else{
                    ESP_LOGE("ota_uart:","重发机制->发送固件包失败:接收方未返回应答信号");
                    success_flag = false;  // 设置失败标志
                }
            }
            if (success_flag == false) 
            {
                // 如果标志为false，表示在重新发送的过程中都未成功接收到应答信号
                return ESP_FAIL;
            }
        }
        // 固件传输进度计算
        int Send_percentage = (int)(((double)(i+1) / (double)firmware_split_number) * (double)100.00);
        ESP_LOGI("ota_uart:","当前传输进度:%d%%",Send_percentage);
        // mqtt_topic_DevToIotOTAUpgradeProgress(9,Send_percentage);
		// 释放分配的内存
        free(uart_rx_data);
        free(firmware_block);
    }
    ESP_LOGI("ota_uart:","当前传输进度:%d%%",100);
    // mqtt_topic_DevToIotOTAUpgradeProgress(9,100);
    ESP_LOGE("ota_uart:","固件已发送完成...");
    return ESP_OK;
}




// ------------------------- cmd_id -------------------------
// 0x00 双方通用的应答包 数据长度 1 数据区 0xff nack
// 0x01 esp询问stm32的版本信息                  nack
// 0x01 stm32应答版本信息                       nack
// 0x02 esp让stm进入bool模式。                  ack
// 0x02 stm进入bool后发送esp已经进入            ack
// 0x03 esp将固件拆分成N/128分发送到stm32       ack
// ------------------------- cmd_id -------------------------

void ota_uart_data_handle(char *uart_data, int length)
{
    ESP_LOGI("ota_uart:","length:%d",length);
    if(uart_data[0] != UART_DATA_HEAD_1 || uart_data[1] != UART_DATA_HEAD_2)
    {
        ESP_LOGE("ota_uart:","串口ota数据包头异常\r\n");
        return;
    }
    // 通过串口包的数据计算出来的校验和
    uint16_t analysis_crc = calculateCRC(uart_data, length - 2); // 将读取到的数据计算校验和
    uint8_t analysis_crc_low = (analysis_crc & 0xFF);
    uint8_t analysis_crc_high = ((analysis_crc >> 8) & 0xFF);
    // 从串口包读取的校验和
    uint8_t uart_crc_low = uart_data[length-2];
    uint8_t uart_crc_high = uart_data[length-1];
    ESP_LOGI("ota_uart:","通过串口数据算出来的校验和 %02x %02x",  analysis_crc_low , analysis_crc_high);
    ESP_LOGI("ota_uart:","读取串口收到的校验和 %02x %02x",  uart_crc_low, uart_crc_high);

    if(analysis_crc_low != uart_crc_low || analysis_crc_high != uart_crc_high)
    {
        ESP_LOGE("ota_uart:","校验和异常\r\n");
        return;
    }

    char cmd_id = uart_data[2];
    char isack = uart_data[3]; 
    uint16_t data_length = ((uart_data)[4] << 8) | (uart_data)[5];  // 数据区长度

    switch (cmd_id)
    {
        case 0x00:
                ESP_LOGI("ota_uart:","双方通用的应答包命令\r\n"); 
                xSemaphoreGive(uart_ota_wait_ack_semap);
            break;
        case 0x01:
                ESP_LOGI("ota_uart:","版本信息命令\r\n"); 
            break;
        case 0x02:
                ESP_LOGI("ota_uart:","bool模式命令\r\n"); 
            break;
        case 0x03:
                //  固件包命令固件包 data[data_length] = 第几个固件包 
                // data[data_length+1]开始才是固件数据
                ESP_LOGI("ota_uart:","固件包命令\r\n"); 
            break;
        default:
            break;
    }
}

static void uart_select_task()
{
    // 串口基础配置
    uart_config_t uart_config = 
	{
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    uint8_t *data_u = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, data_u, BUF_SIZE, 20 / portTICK_RATE_MS);
        // // Write data back to the UART
        // uart_write_bytes(UART_NUM_0, (const char *) data, len);
        if(len > 0)
        {
            ota_uart_data_handle((char*)data_u, len);
        }
    }

    vTaskDelete(NULL);
}
/**
 @brief 串口驱动初始化
 @param 无
 @return 无
*/
void UART_Init(void)
{
    uart_ota_wait_ack_semap =  xSemaphoreCreateBinary();    // 创建一个二值信号量用于等待串口ato

    // 创建一个uart任务处理事件
    xTaskCreate(uart_select_task, "uart_select_task", 4*1024, NULL, 5, NULL);
}
void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG,"Version: %s\n", cJSON_Version());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    UART_Init();

    
    // // char* str = ota_OTAUpgradeProgressToJSON(1,1);
    // // ESP_LOGI(TAG,"%s",str);
    // // cJSON_free(str);
    static char buff[2000] = {0};
    memset(buff, '\0', 2000);

    ota_DevInfoVer_str(buff, "1","2","3", 0,"4");
    ESP_LOGI(TAG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s",buff);
    // // ESP_LOGI(TAG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s  \r\n %d",temo,strlen(temo));

    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
    vTaskDelay(1000);


    // 解析涂鸦ota固件json数据
    tuya_ota_info tuy_otaInfo = {.channel = 0,.time = 0, .url = malloc(800), .version = malloc(30)};
    memset(tuy_otaInfo.url, '\0', 800);
    memset(tuy_otaInfo.version, '\0', 30);
    ota_readIotIssueData(&tuy_otaInfo,"{\"data\":{\"size\":\"4496\",\"cdnUrl\":\"https://images.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"hmac\":\"550BA7BA04C010C7F793959E0CB0A2D1B093B82DB2F35D37398AB31CF1B57C84\",\"channel\":9,\"upgradeType\":0,\"execTime\":0,\"httpsUrl\":\"https://fireware.tuyacn.com:1443/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"version\":\"0.0.1\",\"url\":\"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"md5\":\"949c276b81e0f01bf722d2c1320800f0\"},\"msgId\":\"981390931722113025\",\"time\":1705973278,\"version\":\"1.0\"}");
    // 使用解析出来的url固件下载下来后利用串口分包发送
    http_files_data myData;
    http_dowm_files(&myData,"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin");
    ota_send_firmware(&tuy_otaInfo,&myData);



}
