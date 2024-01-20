#include "LmCloudSdk.h"
#include "esp_log.h"

struct  Cloud_Request{
    int t ;
    int status;
    char* data;
} cloud_request;

// 获取云平台请求参数 json字符串
char*  get_connect_request_auth_param(char* sn,char* secretKey){
    /* 创建一个JSON数据对象(链表头结点) -> {} */
    cJSON *cjson_auth_body = cJSON_CreateObject();
    /* 添加一条整数类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_auth_body, "t", 1);
    /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
    cJSON_AddStringToObject(cjson_auth_body, "sn", sn);
    /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
    cJSON_AddStringToObject(cjson_auth_body, "secretKey", secretKey);

    char* ret = cJSON_Print(cjson_auth_body);

    cJSON_Delete(cjson_auth_body);

    return ret;
}
// 获取推送数据 JSON字符串 废弃
char* get_push_data_param_str(void){
    cJSON *cjson_body = cJSON_CreateObject();

    cJSON_AddNumberToObject(cjson_body, "t", 3);
    
    cJSON *cjson_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(cjson_data,"tag1",11);
    cJSON_AddNumberToObject(cjson_data,"tag2",11);
    cJSON_AddNumberToObject(cjson_data,"tag3",11);
    // 将data添加到json主体里面
    cJSON_AddItemToObject(cjson_body, "data", cjson_data);

    char* ret = cJSON_Print(cjson_body);

    cJSON_Delete(cjson_body);
    cJSON_Delete(cjson_data);
    return ret;
}
// 服务器请求数据处理  传入dataJSON字符串  传入响应结构体去存储数据
void request_data_json_parse(char* data_str, struct cloud_request* cr){
    // 第一时间把cr清空一下
    // cr = {-1,-1,"","",""};

    cJSON* data_json = cJSON_Parse(data_str);
    if(data_json == NULL)
    {
        cJSON_Delete(data_json);
        return;
    }

    cJSON* t_data_val_json = cJSON_GetObjectItem(data_json,"t");
    if(t_data_val_json == NULL){
        cJSON_Delete(t_data_val_json);
        return;
    }
    int t = t_data_val_json->valueint;

    // ESP_LOGI("json","-------拿到t->%d-----开始处理数据",t_data_val_json->valueint);

    int status = -1;
    // 用于连接状态响应 和 上报数据确认
    cJSON* status_data_val_json = NULL;
    switch(t){
        
        // 连接响应
        case 2:
            status_data_val_json = cJSON_GetObjectItem(data_json,"status");
            if(status_data_val_json == NULL){
                return;
            }
            status = status_data_val_json->valueint;
            cr->t=t;
            cr->status= status ;

            // ESP_LOGI("json","连接响应的json数据");

            break;
        // 上报数据确认
        case 4:
            status_data_val_json = cJSON_GetObjectItem(data_json,"status");
            if(status_data_val_json == NULL){
                cJSON_Delete(t_data_val_json);
                return;
            }else{}
            status = status_data_val_json->valueint;

            cr->t=t;

            cr->status= status;
            if(status == 0){
                ESP_LOGI("json","上报数据回调 --> success");
            }
            else{
                ESP_LOGI("json","上报数据回调 --> error");
            }
            break;
        // 命令请求
        case 5:
            // 给t赋值
            cr->t=t;
            // 获取cmdid
            cJSON* cmdId_val_j = cJSON_GetObjectItem(data_json,"cmdId");
            if(cJSON_IsString(cmdId_val_j) == true){
                free(cr->cmdid);
                cr->cmdid = strdup(cmdId_val_j->valuestring);
            } else{}
            // 建议写上else 

            // 获取标识符
            cJSON* apitag_val_j = cJSON_GetObjectItem(data_json,"apitag");
            cr->apitag = apitag_val_j->valuestring;

            // 获取数据
            cJSON* data_val_j = cJSON_GetObjectItem(data_json,"data");
            cr->data = data_val_j->valueint;

            ESP_LOGI("json","云平台命令请求的JSON数据");
            cJSON_Delete(cmdId_val_j);
            cJSON_Delete(apitag_val_j);
            cJSON_Delete(data_val_j);
            break;
    }
    cJSON_Delete(t_data_val_json);
    if(status_data_val_json != NULL){
        cJSON_Delete(status_data_val_json);
    }
    cJSON_Delete(data_json);
}