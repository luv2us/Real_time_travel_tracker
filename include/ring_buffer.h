#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>
#define ADC_READ_LEN 504
#define RING_BUFFER_SIZE (ADC_READ_LEN * 3)
#define STREAM_BUFFER_SIZE (ADC_READ_LEN * 3 * sizeof(int16_t)) // Size in bytes

// Original ring buffer implementation
typedef struct
{
    int16_t *buffer;
    size_t write_ptr;
    size_t read_ptr;
    size_t size;
    SemaphoreHandle_t lock;
} ring_buffer_t;

// Stream buffer handle
extern StreamBufferHandle_t stream_buffer;

// Original ring buffer functions
void ring_buffer_create(void);
size_t ring_buffer_get_free_size(void);
bool ring_buffer_write(const int16_t *data, size_t len);
size_t ring_buffer_read(int16_t *data, size_t len);
extern ring_buffer_t audio_buffer;

// Stream buffer functions
void stream_buffer_create(void);
size_t stream_buffer_get_free_size(void);
size_t stream_buffer_write(const int16_t *data, size_t len);
size_t stream_buffer_read(int16_t *data, size_t len);

#endif
