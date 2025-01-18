#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>
#define RING_BUFFER_SIZE (10240 * 3)

typedef struct
{
    int16_t *buffer;
    size_t write_ptr;
    size_t read_ptr;
    size_t size;
    SemaphoreHandle_t lock;
} ring_buffer_t;

void ring_buffer_create(void);
size_t ring_buffer_get_free_size(void);
bool ring_buffer_write(const int16_t *data, size_t len);
size_t ring_buffer_read(int16_t *data, size_t len);
extern ring_buffer_t audio_buffer;
#endif