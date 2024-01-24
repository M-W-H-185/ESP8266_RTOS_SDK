#pragma once
#include "esp_err.h"

typedef struct http_files_data_t{
    int     readData_count; // 接受文件的计数器
    void*   data;
}http_files_data;
esp_err_t http_dowm_files(http_files_data *hf_data,char* url);


