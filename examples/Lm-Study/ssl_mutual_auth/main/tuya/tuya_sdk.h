#pragma once

// 存涂鸦ota传过来的固件信息
typedef struct tuya_ota_info_t
{
    int     time;
    int     channel;
    char*   version;    // 版本号
    char*   url;
}tuya_ota_info;


// 返回ota升级进度json字符串
char* ota_OTAUpgradeProgressToJSON(int channel,int progress);

//  返回ota设备的字符串
void ota_DevInfoVer_str(char *buff, char *bizType, char *pid, char *firmwareKey, int channel, char *version);

// 解析云平台发过来的固件json 利用url下载固件
void ota_readIotIssueData(tuya_ota_info *tuy_otaInfo, char* issuedata_str);

