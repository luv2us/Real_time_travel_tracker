#ifndef _KEY_H_
#define _KEY_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#define HIGH 1
#define LOW  0
#define Key_Sta  GPIO_NUM_16 // DATA 
#define Key_RD   GPIO_NUM_7   // PL
#define Key_CLK  GPIO_NUM_15
#define numofkey GPIO_NUM_7
#define PL_L gpio_set_level(Key_RD, LOW)
#define PL_H gpio_set_level(Key_RD, HIGH)
#define CLK_L gpio_set_level(Key_CLK, LOW)
#define CLK_H gpio_set_level(Key_CLK, HIGH)

// 定义键值
#define KEY_UP     239
#define KEY_DOWN   191
#define KEY_ENTER  223
#define KEY_BACK   247
#define KEY_VOICE  251
#define KEY_POWER  126
#define KEY_SEARCH 253

typedef enum
{
    KEY_NONE = 0,
    KEY_SHORT_PRESS = 1,
    KEY_LONG_PRESS = 2,
} key_state_t;
typedef struct
{
    // elab_device_t *dev;
    uint32_t press_start_time;
    key_state_t status;
    bool is_pressed;
} elab_key_t;

extern elab_key_t keys[numofkey];
void key_init(void);
void key_scan(void *parameter);
// uint8_t ReadByte_165(void);
// void Latch_165(void);
#endif