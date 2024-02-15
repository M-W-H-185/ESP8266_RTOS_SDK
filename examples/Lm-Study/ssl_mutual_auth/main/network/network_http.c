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
                int num = atoi(evt->header_value);  // 字符串转数字
                ESP_LOGI(TAG, "Content-Length = %d \n", num);
                if(myData->is_dowm == 1) 
                {
                    myData->files_size = num;
                }
            }
            else
            {
                // ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // 将接收到的数据拷贝到缓冲区中
                if(myData->data != NULL && myData->is_dowm == 2)
                {
                    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d this_readData_count %d\n", evt->data_len,myData->readData_count);
                    memcpy(
                        myData->data +  myData->readData_count, 
                        evt->data, 
                        evt->data_len
                    );
                    myData->readData_count += evt->data_len;
                }
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
esp_err_t http_dowm_files(http_files_data *hf_data,char* url,int range_start, int range_end)
{
    if(hf_data->data == NULL)
    {
        return ESP_FAIL;
    }
    
    esp_err_t err = ESP_FAIL;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = hf_data
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(range_end > 0)
    {
        char k_buff[35] = {0x00};
        sprintf(k_buff, "bytes=%d-%d", range_start,range_end);
        esp_http_client_set_header(client,"Range",k_buff);
        ESP_LOGI("http_dowm:","%s\r\n",k_buff);
    }



    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "File downloaded successfully\n");
        for (int i = 0; i < hf_data->readData_count; i++)
        {
            printf("%02x ",((char*)hf_data->data)[i]);
        }
        printf("\r\n");
    } else {
        ESP_LOGE(TAG, "File download failed\n");
     
    }

    esp_http_client_cleanup(client);
    return err;
}
