#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"


#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"

#include <lwip/netdb.h>
#include "LmWifi.h"
#include "LmCloudSdk.h"

void lm_tcp_init(void);
// static char* Lm_Auth_Params = "{\"t\":1,\"sn\":\"sdg345tbdfbg3yg3q\",\"secretKey\":\"asdfqwrewzzxcvwqerdsv32r4t3rgefvc23\"}";
