#include "network_mqtt_task.h"
#include "mqtt_client.h"
#include "esp_err.h"
#include "esp_log.h"
#include "tuya_sdk.h"
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
                                                "{\"msgId\":\"45lkj3551234***\",\"time\":1626197189638,\"data\":{\"temperature\":{\"value\":33,\"time\":1626197189638},\"switch_led\":{\"value\":true,\"time\":1626197189638}}}",
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

esp_mqtt_client_handle_t mqtt_client; 
void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://m1.tuyacn.com:8883",
        .username = "2682965a7c28aaee3b0zdk|signMethod=hmacSha256,timestamp=1705568057,secureMode=1,accessType=1",
        .password = "4551c26bf43848472a5791d968cad7d0091fdeaf4b263a33ba606a29d3364b5c",
        .client_id = "tuyalink_2682965a7c28aaee3b0zdk",
        .event_handle = mqtt_event_handler,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(mqtt_client);  // 启动mqtt任务
}
