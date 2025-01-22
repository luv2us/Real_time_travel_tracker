#ifndef _MY_AUDIO_H_
#define _MY_AUDIO_H_

#include <stdio.h>
#include <math.h>
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "raw_data.h"

// 音频配置
#define SAMPLE_RATE 8000 // 采样率
#define AMPLITUDE 32767  // 16位音频最大振幅
#define PI 3.14159265358979323846
#define I2S_NUM I2S_NUM_1   // 使用 I2S1
#define BUFFER_SIZE 1024    // 缓冲区大小
#define I2S_BUFFER_SIZE 512 // I2S流缓冲区大小

// I2S GPIO 配置
#define I2S_BCK_PIN GPIO_NUM_38  // I2S 位时钟
#define I2S_WS_PIN GPIO_NUM_40   // I2S 字选择线
#define I2S_DOUT_PIN GPIO_NUM_39 // I2S 数据输出
#define GPIO_OUTPUT_PIN GPIO_NUM_41

// I2S 函数声明
void i2s_init();
void i2s_send_audio();
void generate_stereo_sine_wave(int16_t *buffer, size_t size, float frequency);

extern i2s_chan_handle_t tx_chan;

#endif
