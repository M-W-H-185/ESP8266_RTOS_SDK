/* gpio example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "Test_Task";

static void Test_Task(){
    int count = 0 ;
    while (1)
    {
        count ++;
        ESP_LOGI(TAG, "%d \r\n", count);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

}
void app_main(void)
{
    xTaskCreate(Test_Task, "Test_Task", 1024, NULL, 5, NULL);    
    return;
}


