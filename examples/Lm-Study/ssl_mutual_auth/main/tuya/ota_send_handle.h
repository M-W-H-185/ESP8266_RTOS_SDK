#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "tuya_sdk.h"
#include "network_http.h"
#define BUF_SIZE (1024)


// 串口帧头
#define UART_DATA_HEAD_1 0xED
#define UART_DATA_HEAD_2 0x90

extern SemaphoreHandle_t uart_ota_wait_ack_semap ;
extern SemaphoreHandle_t uart_ota_wait_dev_to_bl ;
uint16_t calculateCRC(char *data, int length) ;

esp_err_t ota_http_dwom_and_send_firmwaer(tuya_ota_info *tuya_otoInfo);


