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
#include "cJSON.h"
static const char *TAG = "MQTTS_EXAMPLE";


typedef struct mqtt_topic_config_t{
    char qos;
    const char *topic;
}mqtt_topic_config;
 // 设备向平台发送数据
static const mqtt_topic_config topic_devToIotData = {.qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/thing/property/report"};
  // 云平台控制设备
static const mqtt_topic_config topic_IotControlToDevData = { .qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/thing/property/set"};

static const mqtt_topic_config topic_DevToIotOTAInfoVer = { .qos = 0, .topic = "tylink/2682965a7c28aaee3b0zdk/ota/firmware/report"};

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
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(
                                                client, topic_devToIotData.topic, 
                                                "{\"msgId\":\"45lkj3551234***\",\"time\":1626197189638,\"data\":{\"temperature\":{\"value\":32,\"time\":1626197189638},\"switch_led\":{\"value\":true,\"time\":1626197189638}}}",
                                                0, topic_devToIotData.qos, 0
                                            );
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            
            char* str =  ota_DevInfoVer_str("INIT","l5jc92kpr20a1wcn","keywrsh88jjv99pr",0,"0.0.0");
            ESP_LOGI(TAG, "%s\r\n", str);
            msg_id = esp_mqtt_client_publish( client, topic_DevToIotOTAInfoVer.topic, str, 0, topic_DevToIotOTAInfoVer.qos, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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
        // .client_cert_pem = (const char *)client_cert_pem_start,
        // .client_key_pem = (const char *)client_key_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
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
    
    char* str = NULL;
    str = ota_DevInfoVer_str("111","222","333",444,"555");

    ESP_LOGI(TAG, "%s\r\n", str);
    cJSON_free(str);
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
