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
#include "uart.h"
static const char *TAG = "MAIN";

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief 串口驱动初始化
 @param 无
 @return 无
*/
void UART_Init(void)
{
	// Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = 
	{
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_0, &uart_config);											// 配置串口0参数

    // Install UART driver, and get the queue.
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, &s_uart0Queue, 0);		// 安装UART驱动程序

    // Create a task to handler UART event from ISR
    xTaskCreate(uartEventTask, "uartEventTask", 2048, NULL, 12, NULL);	
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

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    http_files_data *myData;
    myData = malloc(sizeof(http_files_data)); // 为结构体指针分配内存
    http_dowm_files(myData,"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin");

    mqtt_app_start();
}
