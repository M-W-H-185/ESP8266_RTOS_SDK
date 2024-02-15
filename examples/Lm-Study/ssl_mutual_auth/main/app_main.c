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
#include "ota_send_handle.h"
#define UART_MAX_NUM_RX_BYTES (255+4)

static const char *TAG = "MAIN";


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
    for (size_t i = 0; i < length; i++)
    {
        printf("%02x ",uart_data[i]);
    }
    printf("\r\n");
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
                char mode = uart_data[6];
                char v_length = uart_data[7];   // 字符串长度
                ESP_LOGI("ota_uart:","App版本字符串长度%d \r\n",v_length);
                char v_str[100] = {0};
                memset(v_str,0,100);
                memcpy(v_str, (char*)&uart_data[8], v_length);
                if(mode == 0x01)    // BootLoader
                {
                    xSemaphoreGive(uart_ota_wait_dev_to_bl);
                    ESP_LOGI("ota_uart:","当前处于设备BootLoader模式:%s\r\n",v_str);
                } 
                else
                {
                    ESP_LOGI("ota_uart:","当前处于设备App模式:%s\r\n",v_str);
                    mqtt_topic_DevToIotOTAInfoVer(9,v_str);
                }
               
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
    uart_ota_wait_dev_to_bl =  xSemaphoreCreateBinary();  
    
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
    // static char buff[2000] = {0};
    // memset(buff, '\0', 2000);

    // ota_DevInfoVer_str(buff, "1","2","3", 0,"4");
    // ESP_LOGI(TAG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s",buff);
    // // // ESP_LOGI(TAG,"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%s  \r\n %d",temo,strlen(temo));


    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
    vTaskDelay(105);

    tuya_ota_info tuy_otaInfo = {
        .channel = 9,
        .time = 1707916580, 
        .url = "http://note-xm.oss-cn-guangzhou.aliyuncs.com/otatest/stm32_app_v0.0.1.bin",
        .version = "0.0.1",
    };
    ota_http_dwom_and_send_firmwaer(&tuy_otaInfo);

    // // // 解析涂鸦ota固件json数据
    // tuya_ota_info tuy_otaInfo = {.channel = 0,.time = 0, .url = malloc(800), .version = malloc(30)};
    // memset(tuy_otaInfo.url, '\0', 800);
    // memset(tuy_otaInfo.version, '\0', 30);
    // // ota_readIotIssueData(&tuy_otaInfo,"{\"data\":{\"size\":\"7960\",\"cdnUrl\":\"https://images.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/1706768807678190c9677.bin\",\"hmac\":\"74C1C88337F71A8F1228FFEEE18EC386A8DD6381C93F247D782BA85894EB6DF8\",\"channel\":9,\"upgradeType\":0,\"execTime\":0,\"httpsUrl\":\"https://fireware.tuyacn.com:1443/smart/firmware/upgrade/bay1705565836444xBls/1706768807678190c9677.bin\",\"version\":\"0.0.1\",\"url\":\"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/1706768807678190c9677.bin\",\"md5\":\"e11f5cc302aca5721358b48f2a19a156\"},\"msgId\":\"984734436576792577\",\"time\":1706770431,\"version\":\"1.0\"}");
    // // // 使用解析出来的url固件下载下来后利用串口分包发送
    // http_files_data myData;
    // http_dowm_files(&myData,"http://note-xm.oss-cn-guangzhou.aliyuncs.com/otatest/stm32_app_v0.0.1.bin");
    // ota_send_firmware(&tuy_otaInfo,&myData);

}
