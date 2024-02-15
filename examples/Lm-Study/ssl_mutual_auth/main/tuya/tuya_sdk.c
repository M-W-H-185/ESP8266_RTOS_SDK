
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "tuya_sdk.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cJSON.h"
static const char *TAG = "TUYA_SDK";
esp_err_t ota_OTAUpgradeProgressToJSON(char *buff, int channel, int progress)
{
    // {
    //     "msgId":"45lkj3551234***",
    //     "time":1626197189638,
    //     "data":{
    //         "channel":0,
    //         "progress":98
    //     }
    // }
    esp_err_t ret = ESP_FAIL;
    if(buff == NULL)
    {
        ret = ESP_FAIL;
        return ret;
    }
    // 创建外层 JSON 对象
    cJSON *root = cJSON_CreateObject();
    // 添加 "msgId" 字段
    cJSON_AddStringToObject(root, "msgId", "");
    // 添加 "time" 字段
    cJSON_AddNumberToObject(root, "time", 167189638);
    // 创建内层 JSON 对象 "data"
    cJSON *dataObj = cJSON_CreateObject();
    // 添加 "channel" 字段
    cJSON_AddNumberToObject(dataObj, "channel", channel);
    // 添加 "progress" 字段
    cJSON_AddNumberToObject(dataObj, "progress", progress);
    // 将内层 JSON 对象添加到外层对象中
    cJSON_AddItemToObject(root, "data", dataObj);
    // 将 JSON 对象转换为字符串并复制到传入的 buff 中
    if(strlen(cJSON_Print(root)) > sizeof(buff))
    {
        strcpy(buff, cJSON_Print(root));
        ret = ESP_OK;
        cJSON_Delete(root);
        return ret;
    }
    // 释放内存
    cJSON_Delete(root);
    return ret;
}
//  返回ota设备的JSON字符串
void ota_DevInfoVer_str(char*buff,char *bizType, char *pid, char *firmwareKey, int channel, char *version)
{
    const char* msgId = "45lkj3551234***";
    int time = 2343345;

    // 创建根JSON对象
    cJSON *rootObject = cJSON_CreateObject();

    // 添加msgId和time到根JSON对象
    cJSON_AddItemToObject(rootObject, "msgId", cJSON_CreateString(msgId));
    cJSON_AddItemToObject(rootObject, "time", cJSON_CreateNumber(time));

    // 创建data对象并添加到根JSON对象
    cJSON *dataObject = cJSON_CreateObject();
    cJSON_AddItemToObject(rootObject, "data", dataObject);

    // 添加业务类型、产品ID和固件密钥到data对象
    cJSON_AddItemToObject(dataObject, "bizType", cJSON_CreateString(bizType));
    cJSON_AddItemToObject(dataObject, "pid", cJSON_CreateString(pid));
    cJSON_AddItemToObject(dataObject, "firmwareKey", cJSON_CreateString(firmwareKey));

    // 创建otaChannel数组并添加到data对象
    cJSON *otaChannelArray = cJSON_CreateArray();
    cJSON_AddItemToObject(dataObject, "otaChannel", otaChannelArray);

    // 添加第一个渠道信息到otaChannel数组
    cJSON *otaChannelItem1 = cJSON_CreateObject();
    cJSON_AddItemToObject(otaChannelItem1, "channel", cJSON_CreateNumber(channel));
    cJSON_AddItemToObject(otaChannelItem1, "version", cJSON_CreateString(version));
    cJSON_AddItemToArray(otaChannelArray, otaChannelItem1);


    // 生成JSON字符串
    char *jsonString = cJSON_Print(rootObject);

    // 使用 memcpy 将 JSON 字符串复制到输出缓冲区
    memcpy(buff, jsonString, strlen(jsonString) + 1);  // 包括字符串结尾的'\0'

    // 释放JSON对象和生成的JSON字符串的内存
    cJSON_Delete(rootObject);
    free(jsonString);

}
// 解析云平台发过来的固件json 利用url下载固件
void ota_readIotIssueData(tuya_ota_info *tuy_otaInfo, char* issuedata_str)
{
    cJSON* issuedata_body = cJSON_Parse(issuedata_str);
    cJSON* issuedata_body_time = cJSON_GetObjectItem(issuedata_body,"time");

    cJSON* issuedata_body_data = cJSON_GetObjectItem(issuedata_body,"data");

    cJSON* issuedata_body_data_channel = cJSON_GetObjectItem(issuedata_body_data,"channel");
    cJSON* issuedata_body_data_url = cJSON_GetObjectItem(issuedata_body_data,"url");
    cJSON* issuedata_body_data_version = cJSON_GetObjectItem(issuedata_body_data,"version");

    tuy_otaInfo->time = issuedata_body_time->valueint;
    tuy_otaInfo->channel = issuedata_body_data_channel->valueint;

    ESP_LOGI(TAG, "1 time:%d", tuy_otaInfo->time);
    ESP_LOGI(TAG, "2 channel:%d", tuy_otaInfo->channel);

    memcpy(tuy_otaInfo->url,issuedata_body_data_url->valuestring,strlen(issuedata_body_data_url->valuestring));
    ESP_LOGI(TAG, "3 url:%s", tuy_otaInfo->url);

    memcpy(tuy_otaInfo->version,issuedata_body_data_version->valuestring,strlen(issuedata_body_data_version->valuestring) );
    ESP_LOGI(TAG, "4 version:%s", tuy_otaInfo->version);

    // char *url = issuedata_body_data_url->valuestring;

    cJSON_Delete(issuedata_body);
    cJSON_Delete(issuedata_body_time);
    cJSON_Delete(issuedata_body_data);
    cJSON_Delete(issuedata_body_data_channel);
    cJSON_Delete(issuedata_body_data_url);
    cJSON_Delete(issuedata_body_data_version);
    // return url;
}

