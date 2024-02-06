
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "tuya_sdk.h"
#include "esp_log.h"
#include "cJSON.h"
static const char *TAG = "TUYA_SDK";
char* ota_OTAUpgradeProgressToJSON(int channel,int progress)
{
    // {
    //     "msgId":"45lkj3551234***",
    //     "time":1626197189638,
    //     "data":{
    //         "channel":0,
    //         "progress":98
    //     }
    // }
    // /* 创建一个JSON数据对象(链表头结点) -> {} */
    cJSON *cjson_Boby = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_Boby, "msgId", "45lkj3551234");
    cJSON_AddNumberToObject(cjson_Boby, "time", 1705912127);
    // boby下面的data
    cJSON *cjson_Boby_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(cjson_Boby_data,"channel",channel);
    cJSON_AddNumberToObject(cjson_Boby_data,"progress",progress);
    // 将data放入boby
    cJSON_AddItemToObject(cjson_Boby,"data",cjson_Boby_data);

    char *str = cJSON_Print(cjson_Boby);

    cJSON_Delete(cjson_Boby);
    cJSON_Delete(cjson_Boby_data);

    return str;
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

