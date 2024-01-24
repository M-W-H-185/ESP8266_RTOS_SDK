#include "network_http.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_err.h"


static const char *TAG = "http";


esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_files_data *myData = (http_files_data *)evt->user_data;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR\n");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED\n");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT\n");
            break;
        case HTTP_EVENT_ON_HEADER:
            if(strcmp(evt->header_key,"Content-Length") == 0)
            {
                int num = atoi(evt->header_value);
                ESP_LOGI(TAG, "Content-Length = %d\n", num);
                // ------------------- 分配内存 -------------------
                myData->data = malloc(num);
                // 检查内存分配是否成功
                if (myData->data == NULL) {
                    ESP_LOGI(TAG, "Error: Memory allocation failed.\n");
                    return ESP_FAIL;
                }
                memset(myData->data, 0, num);
                myData->readData_count = 0;
                // ------------------- 分配内存 -------------------
            }
            else
            {
                ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
                // 将接收到的数据拷贝到缓冲区中
                memcpy(
                    myData->data +  myData->readData_count, 
                    evt->data, 
                    evt->data_len
                    );
               myData->readData_count += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH\n");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED\n");
            break;
    }
    return ESP_OK;
}

// http下载文件
esp_err_t http_dowm_files(http_files_data *hf_data,char* url)
{
    esp_err_t err = ESP_FAIL;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = hf_data
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "File downloaded successfully\n");
        for (int i = 0; i < hf_data->readData_count; i++)
        {
            printf("%02x",((char*)hf_data->data)[i]);
        }
        printf("\r\n");

    } else {
        ESP_LOGE(TAG, "File download failed\n");
    }

    esp_http_client_cleanup(client);
    return err;
}
