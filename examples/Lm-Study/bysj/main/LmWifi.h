#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"



#define LM_WIFI_SSID "wlw"
#define LM_WIFI_PASS "12345678"
// 最大重新连接数
#define LM_MAXIMUM_RETRY  5

void wifi_init_sta(void);
// 该方法只能获取第一次连接WiFi的状态，断开WiFi后就不能自动重连 还在看
bool Lm_getIsWifiConnectSuccess(void);