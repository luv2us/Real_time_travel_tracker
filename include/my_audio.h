#ifndef _MY_AUDIO_H_
#define _MY_AUDIO_H_

#include <stdio.h>
#include <math.h>
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wcpp"
// #include "driver/i2s.h"
#include "driver/i2s_std.h"
// #pragma GCC diagnostic pop

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "raw_data.h"
// 定义常量
#define SAMPLE_RATE 8000 // 采样率
#define AMPLITUDE 32767  // 16位有符号音频的最大振幅
#define PI 3.14159265358979323846
#define I2S_NUM I2S_NUM_1 // 使用 I2S1
#define BUFFER_SIZE 1024  // 缓冲区大小

// 定义 GPIO 引脚
#define I2S_BCK_PIN GPIO_NUM_38  // I2S 位时钟
#define I2S_WS_PIN GPIO_NUM_40   // I2S 字选择（左右声道）
#define I2S_DOUT_PIN GPIO_NUM_39 // I2S 数据输出
#define GPIO_OUTPUT_PIN GPIO_NUM_41
// I2S 配置
void i2s_init();
void i2s_send_audio();
void generate_stereo_sine_wave(int16_t *buffer, size_t size, float frequency);
extern i2s_chan_handle_t tx_chan;
#endif