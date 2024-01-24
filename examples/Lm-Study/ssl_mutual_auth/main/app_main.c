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
#include "esp_http_client.h"
#include "tuya_sdk.h"
#include "cJSON.h"
static const char *TAG = "MAIN";



int read_data_length = 0;
char read_data[1024*5] = {0};

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            printf("HTTP_EVENT_ERROR\n");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            printf("HTTP_EVENT_ON_CONNECTED\n");
            break;
        case HTTP_EVENT_HEADER_SENT:
            printf("HTTP_EVENT_HEADER_SENT\n");
            break;
        case HTTP_EVENT_ON_HEADER:
            printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
                // 将接收到的数据拷贝到缓冲区中
                memcpy(read_data + read_data_length, evt->data, evt->data_len);
                read_data_length += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            printf("HTTP_EVENT_ON_FINISH\n");
            break;
        case HTTP_EVENT_DISCONNECTED:
            printf("HTTP_EVENT_DISCONNECTED\n");
            break;
    }
    return ESP_OK;
}


void ota_send_stm32_firmware_updata_task(void *pvParameters)
{
    ota_readIotIssueData("{\"data\":{\"size\":\"4496\",\"cdnUrl\":\"https://images.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"hmac\":\"550BA7BA04C010C7F793959E0CB0A2D1B093B82DB2F35D37398AB31CF1B57C84\",\"channel\":9,\"upgradeType\":0,\"execTime\":0,\"httpsUrl\":\"https://fireware.tuyacn.com:1443/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"version\":\"0.0.1\",\"url\":\"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"md5\":\"949c276b81e0f01bf722d2c1320800f0\"},\"msgId\":\"981390931722113025\",\"time\":1705973278,\"version\":\"1.0\"}");
    ota_DevInfoVer_str("","","",1,"");
    memset(read_data, 0, (1024*5));
    for (int i = 0; i < 5120; i++)
    {
        printf("%02x", read_data[i]);
    }
    esp_http_client_config_t config = {
        .url = "http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin",
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        printf("File downloaded successfully\n");
        for (int i = 0; i < 5120; i++)
        {
            printf("%02x", read_data[i]);
        }
        
    } else {
        printf("File download failed\n");
    }

    esp_http_client_cleanup(client);

    vTaskDelete(NULL);
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
    
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    xTaskCreate(&ota_send_stm32_firmware_updata_task, "ota_send_stm32_firmware_updata_task", 16384, NULL, 5, NULL);

    mqtt_app_start();
}
