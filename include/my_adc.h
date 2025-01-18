#ifndef _MY_ADC_H_
#define _MY_ADC_H_
#include <stdio.h>
#include "esp_log.h"
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "driver/adc_types_legacy.h"
#include "driver/gpio.h"
#include <math.h>
#include "ring_buffer.h"
#include "my_audio.h"
// 定义常量
#define ADC_UNIT ADC_UNIT_1                  // 使用 ADC1
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1 // 单单元模式
#define ADC_CHANNEL ADC_CHANNEL_0            // 使用 ADC1 通道 6 (GPIO34)
#define ADC_SAMPLE_FREQ_HZ 8000              // 采样频率 8kHz
#define ADC_READ_LEN 10240
#define DMA_BUFFER_SIZE 20480
extern adc_continuous_handle_t adc_handle;
extern SemaphoreHandle_t adc_semaphore;
// DMA 缓冲区
extern uint8_t dma_buffer[DMA_BUFFER_SIZE];
extern int16_t raw_adc_value[DMA_BUFFER_SIZE / 4];
extern int16_t i2s_data[DMA_BUFFER_SIZE * 2];
void adc_continuous_init();
void adc_continuous_read_task(void *arg);

void process_adc_data();
extern TaskHandle_t s_task_handle;
// 定义常量
#endif
