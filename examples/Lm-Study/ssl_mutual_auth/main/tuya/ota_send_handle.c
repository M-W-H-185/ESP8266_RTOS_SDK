#include "ota_send_handle.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "tuya_sdk.h"
#include "network_http.h"
#include "driver/uart.h"
#include "network_mqtt_task.h"

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
    static uint8_t cmd_id = 0x00;
    static uint8_t is_ack = 0x02;
    char *uart_rx_data = NULL;
    char data[] = {00};
    int uart_rx_data_length = uart_send_cmdid_data_handle(&uart_rx_data, cmd_id, is_ack, data, 1);
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
// 让设备接入bootloader模式
esp_err_t cmd_send_dev_bootloader_model(tuya_ota_info *tuya_otoInfo,http_files_data *hf_data)
{
    xSemaphoreTake(uart_ota_wait_ack_semap, 0); // 先把标志位释放了再说

    static uint8_t cmd_id = 0x02;
    static uint8_t is_ack = 0x01;
    uint16_t data_size = 
                    sizeof(uint32_t) + sizeof(uint32_t) + // 固件大小 + 固件crc校验
                    sizeof(uint8_t) +   // 版本字符串长度
                    strlen(tuya_otoInfo->version); // 字符串空间
    char *data = malloc(data_size); // 加上crc校验和
    memset(data,0x66,data_size);
    // 固件大小
    data[0] = (uint8_t)((hf_data->readData_count >> 16) & 0xFF);    // 获取高8位
    data[1] = (uint8_t)((hf_data->readData_count >> 8) & 0xFF);     // 获取中8位
    data[2] = (uint8_t)((hf_data->readData_count & 0xFF));          // 获取低8位
    data[3] = 0x00;
    // 固件大小

    // 固件crc校验
    uint16_t firmware_crc = calculateCRC(hf_data->data, hf_data->readData_count);
    data[4] = (uint8_t)((firmware_crc >> 8) & 0xFF);   // 获取高8位
    data[5] = (uint8_t)(firmware_crc & 0xFF);          // 获取低8位
    data[6] = 0x00;                             // 预留位
    data[7] = 0x00;                             // 预留位
    // 固件crc校验

    // 固件版本字符串
             
    data[8] = strlen(tuya_otoInfo->version);
    char *data_versioin_str = &data[9]; // 版本字符串开始
    for (int i = 0; i < data[8]; i++)
    {
        data_versioin_str[i] = tuya_otoInfo->version[i];
    }
    
    // 固件版本字符串
    ESP_LOGI("ota_uart:",
    "cmd_send_dev_bootloader_model->dataSize:%d firmwaerSize:%d firmware_crc:%04x version:%s\r\n",
    data_size,hf_data->readData_count,firmware_crc,data_versioin_str);

    // 发送到串口
    char *uart_rx_data = NULL;
    int uart_rx_data_length = uart_send_cmdid_data_handle(&uart_rx_data, cmd_id, is_ack, data, data_size);
    uart_write_bytes(UART_NUM_0, uart_rx_data, uart_rx_data_length);
    // 输出固件debug
    printf("串口命令debug:");
    for(int i = 0; i < uart_rx_data_length; i++){
        printf("%02x ",((char*)uart_rx_data)[i]);
    }
    printf("\r\n");
    free(data);
    ESP_LOGI("ota_uart:","等待设备进入BootLoader\r\n");

    if(xSemaphoreTake(uart_ota_wait_ack_semap, 4000) == pdTRUE)
    {
        ESP_LOGI("ota_uart:","设备进入BootLoader成功\r\n");
        return ESP_OK;
    }
    else
    {
        ESP_LOGE("ota_uart:","设备进入BootLoader失败\r\n");
        return ESP_FAIL;
    }
}



// 开始分批发送固件包
esp_err_t ota_send_firmware(tuya_ota_info *tuya_otoInfo,http_files_data *hf_data)
{
    if( cmd_send_dev_bootloader_model(tuya_otoInfo,hf_data) != ESP_OK)
    {
        // 进入BootLoader失败
        return ESP_FAIL;
    }

    char* firmware = (char*)hf_data->data;          // 固件数据
    int firmware_size = hf_data->readData_count;    // 固件大小
    // memset(firmware, 0x66, firmware_size);
    // 计算拆分次数
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
        char *firmware_block = malloc(2 + firmware_block_size);// +1 是因为需要加上现在是第几个固件包 
        firmware_block[0] = i ;      // 现在是第i个固件包
        firmware_block[1] = firmware_split_number - 1; // 一共有多少个分包 - 1 从0开始计算 0、1、2、3
        char *temp = &firmware_block[2];
        memcpy( temp, (char *)(firmware+(i * 128)) ,  firmware_block_size * sizeof(char));
        uart_rx_data_length = uart_send_cmdid_data_handle(&uart_rx_data, 0x03, 0x01, firmware_block, firmware_block_size + 2);
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
        mqtt_topic_DevToIotOTAUpgradeProgress(9,Send_percentage);
		// 释放分配的内存
        free(uart_rx_data);
        free(firmware_block);
    }
    ESP_LOGI("ota_uart:","当前传输进度:%d%%",100);
    // mqtt_topic_DevToIotOTAUpgradeProgress(9,100);
    ESP_LOGE("ota_uart:","固件已发送完成...");
    return ESP_OK;
}


