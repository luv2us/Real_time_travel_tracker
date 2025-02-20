#ifndef LCD_ST7789_H
#define LCD_ST7789_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "lcdData.h"
#include "key.h"
#include "my_adc.h"  
#include "esp_sleep.h"  
#include "driver/ledc.h"
#include "esp_adc/adc_cali_scheme.h"
//#include "bsp_ble.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_partition.h>
#include <esp_log.h>
#define ZIKU_PARTITION_LABEL "hz"

/*------------------------------------------------------------------------------------------------------------*/
/*color*/

// RGB565 Color Definitions
#define COLOR_BLACK       0x0000  // 黑色
#define COLOR_WHITE       0xFFFF  // 白色
#define COLOR_RED         0xF800  // 红色
#define COLOR_GREEN       0x07E0  // 绿色
#define COLOR_BLUE        0x001F  // 蓝色
#define COLOR_YELLOW      0xFFE0  // 黄色
#define COLOR_CYAN        0x07FF  // 青色
#define COLOR_MAGENTA     0xF81F  // 品红
#define COLOR_GRAY        0x8410  // 灰色
#define COLOR_LIGHTGRAY   0xC618  // 浅灰色
#define COLOR_DARKGRAY    0x4208  // 深灰色
#define COLOR_ORANGE      0xFC00  // 橙色
#define COLOR_BROWN       0xA145  // 棕色
#define COLOR_PINK        0xF81F  // 粉色
#define COLOR_PURPLE      0x8010  // 紫色
#define COLOR_LIGHTBLUE   0x867F  // 浅蓝色
#define COLOR_LIGHTGREEN  0x87F0  // 浅绿色
#define COLOR_DARKBLUE    0x0010  // 深蓝色
#define COLOR_DARKGREEN   0x03E0  // 深绿色
#define COLOR_GOLD        0xFEA0  // 金色
#define COLOR_SILVER      0xC618  // 银色
#define COLOR_VIOLET      0x911F  // 紫罗兰色
#define COLOR_NAVY        0x0010  // 海军蓝
#define COLOR_TEAL        0x0410  // 青绿

#define startColor 0x3298  // 渐变开始色亮宝蓝
#define endColor   0x5290    // 渐变结束色灰蓝紫


/*------------------------------------------------------------------------------------------------------------*/
// 定义引脚
#define LEDA        GPIO_NUM_8
#define TFT_RST     GPIO_NUM_3  
#define LCD_RS      GPIO_NUM_46  // Display data/command selection pin in 4-line serial interface
#define MISO        GPIO_NUM_13  // LCD通信无需MISO，MISO在lora模块使用
#define LCD_MOSI    GPIO_NUM_11  // MOSI
#define LCD_SCLK    GPIO_NUM_12  // This pin is used to be serial interface clock.
#define LCD_SS1     GPIO_NUM_9   // Chip selection pin ,Low enable ,High disable.

// 135(H)RGB x240(V) （翻转90°）
#define LCD_WIDTH   240
#define LCD_HEIGHT  135

// x偏移量为40，行偏移量为53 
#define COL_OFFSET 40
#define ROW_OFFSET 53

// 复位信号控制宏定义
#define LCD_RESET_START  gpio_set_level(TFT_RST, 0)    // 开始复位,复位信号拉低
#define LCD_RESET_STOP   gpio_set_level(TFT_RST, 1)   // 结束复位,复位信号拉高

// 背光控制宏定义
#define LCD_BACKLIGHT_START  closeBacklight(0)  // 开启背光
#define LCD_BACKLIGHT_STOP   closeBacklight(1)   // 关闭背光

// 片选信号控制宏定义
#define LCD_CS_START    gpio_set_level(LCD_SS1, 0)     // 使能片选,片选信号拉低
#define LCD_CS_STOP     gpio_set_level(LCD_SS1, 1)    // 禁用片选,片选信号拉高

// 数据/命令选择信号控制宏定义
#define LCD_writeCMD    gpio_set_level(LCD_RS, 0)      // 写命令模式,RS信号拉低
#define LCD_writeData   gpio_set_level(LCD_RS, 1)     // 写数据模式,RS信号拉高

#define MODE_NON_OVERLAPPING  0  // 非叠加模式
#define MODE_OVERLAPPING      1  // 叠加模式

#define OPEN 1
#define CLOSE 0

#define TRUE 1
#define FALSE 0

#define CHRG_PIN GPIO_NUM_6  //检测是否在充电的引脚

//PWM相关
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          LEDA  // 例如，ESP32-S3的GPIO8连接LEDA
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT  // 分辨率：8位，0-255
#define LEDC_FREQUENCY          5000  // 5kHz PWM频率

#define BRIGHTNESS_LEVELS 11

#define SCREEN_TIMEOUT_1  (30*1000*1000)     // 第一档，30秒
#define SCREEN_TIMEOUT_2  (60*1000*1000)     // 第二档，1分钟
#define SCREEN_TIMEOUT_3  (120*1000*1000)    // 第三档，2分钟
#define SCREEN_TIMEOUT_4  (180*1000*1000)    // 第四档，3分钟
#define SCREEN_TIMEOUT_5  (300*1000*1000)    // 第五档，5分钟
#define SCREEN_TIMEOUT_6  (0x7FFFFFFFFFFFFFFF) // 第六档，永不休眠 2^63-1 = 9223372036854775807微妙 

#define TALK_START
#define TALK_STOP

#define MIC_ICON_OPEN  Microphone(1)  // 显示麦克风图标
#define MIC_ICON_CLOSE Microphone(0)  // 关闭麦克风图标


// 最终版本定义这个
//#define BEST

void LCD_init(); //初始化
void menuControl();

void test();

#endif