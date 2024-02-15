#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"


void mqtt_app_start(void);
esp_err_t mqtt_topic_DevToIotOTAUpgradeProgress(int channel,int progress);
esp_err_t mqtt_topic_DevToIotOTAInfoVer(int channel,char* v_str);

#ifdef __cplusplus
}
#endif
