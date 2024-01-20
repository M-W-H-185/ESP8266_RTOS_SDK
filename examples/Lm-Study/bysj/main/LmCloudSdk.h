#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
// data这里没做太多处理 固定先
struct cloud_request{
    int t ;
    int status;
    char* cmdid;
    char* apitag;
    // 有可能是json字符串 有可能是 数字等
    int data;
};

char* get_connect_request_auth_param(char* sn, char* secretKey);
char* get_push_data_param_str(void);
/// @brief 解析JSON字符串的
/// @param data_str json字符串
/// @param cr 存入指针地址 然后将处理好的JSON数据分别放进去
void request_data_json_parse(char* data_str, struct cloud_request* cr);
