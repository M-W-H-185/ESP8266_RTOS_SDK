#pragma once


//  返回ota设备的字符串
char* ota_DevInfoVer_str(char *bizType, char *pid, char *firmwareKey, int channel, char *version);
// 解析云平台发过来的固件json 利用url下载固件
char* ota_readIotIssueData(char* issuedata_str);