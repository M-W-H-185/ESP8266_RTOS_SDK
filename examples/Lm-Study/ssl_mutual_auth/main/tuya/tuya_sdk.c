#include "tuya_sdk.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "TUYA_SDK";
//  返回ota设备的字符串
char* ota_DevInfoVer_str(char *bizType, char *pid, char *firmwareKey, int channel, char *version)
{
    // /* 创建一个JSON数据对象(链表头结点) -> {} */
    cJSON *cjson_Boby = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_Boby, "msgId", "45lkj3551234***");
    cJSON_AddNumberToObject(cjson_Boby, "time", 1705912127);
    // boby下面的data
    cJSON *cjson_Boby_data = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_Boby_data,"bizType",bizType);
    cJSON_AddStringToObject(cjson_Boby_data,"pid",pid);
    cJSON_AddStringToObject(cjson_Boby_data,"firmwareKey",firmwareKey);
    // data下面的oatChannel数组
    cJSON *oatChannel = cJSON_CreateArray();
    // oatChannel的元素
    cJSON *oatChannel_obj1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(oatChannel_obj1,"channel",channel);
    cJSON_AddStringToObject(oatChannel_obj1,"version",version);   
    // 将元素存入oatChannel数组内
    cJSON_AddItemToArray(oatChannel,oatChannel_obj1);
    // 将oatChannel数组存入data下
	cJSON_AddItemToObject(cjson_Boby_data, "otaChannel", oatChannel);
    // 将data放入boby
    cJSON_AddItemToObject(cjson_Boby,"data",cjson_Boby_data);

    char* str = NULL;
    str = cJSON_Print(cjson_Boby);

    cJSON_Delete(cjson_Boby);
    cJSON_Delete(cjson_Boby_data);
    cJSON_Delete(oatChannel);
    cJSON_Delete(oatChannel_obj1);

    return str;
}
// 解析云平台发过来的固件json 利用url下载固件
char* ota_readIotIssueData(char* issuedata_str)
{
    cJSON* issuedata_body = cJSON_Parse(issuedata_str);
    cJSON* issuedata_body_data = cJSON_GetObjectItem(issuedata_body,"data");
    cJSON* issuedata_body_data_url = cJSON_GetObjectItem(issuedata_body_data,"url");
    ESP_LOGI(TAG, "url:%s", issuedata_body_data_url->valuestring);
    char *url = issuedata_body_data_url->valuestring;

    cJSON_Delete(issuedata_body_data);
    cJSON_Delete(issuedata_body);
    return url;
}
