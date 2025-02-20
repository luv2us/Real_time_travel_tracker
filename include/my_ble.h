#ifndef _MY_BLE_H_
#define _MY_BLE_H_
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include <esp_gap_ble_api.h>
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

extern char packet[100];
void ble_init(void);
#endif