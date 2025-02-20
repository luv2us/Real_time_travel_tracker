#ifndef _MY_GPS_H
#define _MY_GPS_H
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#define GPS_UART_NUM UART_NUM_2
#define GPS_TXD_PIN GPIO_NUM_17
#define GPS_RXD_PIN GPIO_NUM_18
#define GPS_UART_BUFFER_SIZE 1024
#define NMEA_MAX_LENGTH 100
typedef struct
{
    char devname[10];
    char latitudeStr[20];
    char longitudeStr[20];
} device_data_t;
extern device_data_t gps_data;
extern char nmea[100]; // ???????????? NMEA ????
extern SemaphoreHandle_t gps2lora_semaphore;
extern uint16_t bufferIndex; // ?????????????§Ö????¦Ë??
extern bool gps_changed;
extern char last_parsed_nmea[NMEA_MAX_LENGTH];
float convertToDecimal(char *coordinate, char direction);
void parseGPS(char *msg);
void gps_uart_init(void);
void GPS_Task(void *pvParameters);
uint8_t parseGNGGA(char *msg);
uint8_t parseGNGLL(char *msg);
#endif