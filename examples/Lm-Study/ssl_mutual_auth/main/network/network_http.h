#pragma once
#include "esp_err.h"

typedef struct http_files_data_t{
    int     files_size;     // 总文件大小
    int     readData_count; // 接受文件的计数器
    char*   data;
    int     is_dowm;
}http_files_data;
esp_err_t http_dowm_files(http_files_data *hf_data,char* url,int range_start, int range_end);


