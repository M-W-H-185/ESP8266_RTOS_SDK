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
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_http_client.h"

#include "cJSON.h"
static const char *TAG = "MQTTS_EXAMPLE";


typedef struct mqtt_topic_config_t{
    char qos;
    const char *topic;
}mqtt_topic_config;
// 设备向平台发送数据
static const mqtt_topic_config topic_devToIotData = {.qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/thing/property/report"};
// 设备向云平台发送OTA设备信息  版本之类
static const mqtt_topic_config topic_DevToIotOTAInfoVer = { .qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/ota/firmware/report"};

// 云平台控制设备
static const mqtt_topic_config topic_IotControlToDevData = { .qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/thing/property/set"};
// 云平台向设备发送固件升级包uri
static const mqtt_topic_config topic_IotToDevOTAissue = { .qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/ota/issue"};



//  返回ota设备的字符串
static char* ota_DevInfoVer_str(char *bizType, char *pid, char *firmwareKey, int channel, char *version)
{
    // /* 创建一个JSON数据对象(链表头结点) -> {} */
    cJSON *cjson_Boby = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_Boby, "msgId", "45lkj3551234***");
    cJSON_AddNumberToObject(cjson_Boby, "time", 1705912127);
    // boby下面的data
    cJSON *cjson_Boby_data = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_Boby_data,"bizType",bizType);
    cJSON_AddStringToObject(cjson_Boby_data,"pid",pid);
    cJSON_AddStringToObject(cjson_Boby_data,"firmwareKey",firmwareKey);
    // data下面的oatChannel数组
    cJSON *oatChannel = cJSON_CreateArray();
    // oatChannel的元素
    cJSON *oatChannel_obj1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(oatChannel_obj1,"channel",channel);
    cJSON_AddStringToObject(oatChannel_obj1,"version",version);   
    // 将元素存入oatChannel数组内
    cJSON_AddItemToArray(oatChannel,oatChannel_obj1);
    // 将oatChannel数组存入data下
	cJSON_AddItemToObject(cjson_Boby_data, "otaChannel", oatChannel);
    // 将data放入boby
    cJSON_AddItemToObject(cjson_Boby,"data",cjson_Boby_data);

    char* str = NULL;
    str = cJSON_Print(cjson_Boby);

    cJSON_Delete(cjson_Boby);
    cJSON_Delete(cjson_Boby_data);
    cJSON_Delete(oatChannel);
    cJSON_Delete(oatChannel_obj1);

    return str;
}
// 解析云平台发过来的固件json 利用url下载固件
static char* ota_readIotIssueData(char* issuedata_str)
{
    cJSON* issuedata_body = cJSON_Parse(issuedata_str);
    cJSON* issuedata_body_data = cJSON_GetObjectItem(issuedata_body,"data");
    cJSON* issuedata_body_data_url = cJSON_GetObjectItem(issuedata_body_data,"url");
    ESP_LOGI(TAG, "url:%s", issuedata_body_data_url->valuestring);
    char *url = issuedata_body_data_url->valuestring;

    cJSON_Delete(issuedata_body_data);
    cJSON_Delete(issuedata_body);
    return url;
}
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            msg_id = esp_mqtt_client_subscribe(client, topic_IotControlToDevData.topic, topic_IotControlToDevData.qos);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, topic_IotToDevOTAissue.topic, topic_IotToDevOTAissue.qos);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // 推送传感器信息
            msg_id = esp_mqtt_client_publish(
                                                client, topic_devToIotData.topic, 
                                                "{\"msgId\":\"45lkj3551234***\",\"time\":1626197189638,\"data\":{\"temperature\":{\"value\":32,\"time\":1626197189638},\"switch_led\":{\"value\":true,\"time\":1626197189638}}}",
                                                0, topic_devToIotData.qos, 0
                                            );
            ESP_LOGI(TAG, "11111111111111111111sent publish successful, msg_id=%d", msg_id);
            // 推送ota信息
            char* str =  ota_DevInfoVer_str("INIT","l5jc92kpr20a1wcn","keywrsh88jjv99pr",9,"0.0.0");
            msg_id = esp_mqtt_client_publish( client, topic_DevToIotOTAInfoVer.topic, str, 0, topic_DevToIotOTAInfoVer.qos, 0);
            ESP_LOGI(TAG, "22222222222222222222sent publish successful, msg_id=%d", msg_id);
            cJSON_free(str);
            
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            // mqtt 收到数据事件
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://m1.tuyacn.com:8883",
        .username = "2682965a7c28aaee3b0zdk|signMethod=hmacSha256,timestamp=1705568057,secureMode=1,accessType=1",
        .password = "4551c26bf43848472a5791d968cad7d0091fdeaf4b263a33ba606a29d3364b5c",
        .client_id = "tuyalink_2682965a7c28aaee3b0zdk",
        .event_handle = mqtt_event_handler,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}
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

void download_file_task(void *pvParameters)
{
    ota_readIotIssueData("{\"data\":{\"size\":\"4496\",\"cdnUrl\":\"https://images.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"hmac\":\"550BA7BA04C010C7F793959E0CB0A2D1B093B82DB2F35D37398AB31CF1B57C84\",\"channel\":9,\"upgradeType\":0,\"execTime\":0,\"httpsUrl\":\"https://fireware.tuyacn.com:1443/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"version\":\"0.0.1\",\"url\":\"http://airtake-public-data-1254153901.cos.tuyacn.com/smart/firmware/upgrade/bay1705565836444xBls/17059072757e724aa563f.bin\",\"md5\":\"949c276b81e0f01bf722d2c1320800f0\"},\"msgId\":\"981390931722113025\",\"time\":1705973278,\"version\":\"1.0\"}");

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
    xTaskCreate(&download_file_task, "download_file_task", 16384, NULL, 5, NULL);

    mqtt_app_start();
}
