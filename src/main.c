#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "my_adc.h"
#include "LCD_ST7789.h"
#include "soc/soc_caps.h"
#include "ring_buffer.h"
#include "my_audio.h"
#include "my_ble.h"
#include "my_gps.h"
#include "lora.h"
/*
    1. 音频还原调试 多一个通道？环形缓冲区写入字节？

    2. 音频数组的拆分与解析

    3.lora数据的识别（发与收的切换？）

    4.  使用polling实现lora传输，优化：考虑队列传输大量数据
*/

int16_t read_from_ringbuffer[ADC_READ_LEN];
void ringbuffer_read_test(void *pvParameters)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        size_t read_len = ring_buffer_read(read_from_ringbuffer, (ADC_READ_LEN / 2));

        if (read_len > 0)
        {
            //  esp_rom_printf("read_len: %d\n", read_len);
            esp_err_t ret = i2s_channel_write(tx_chan, read_from_ringbuffer, read_len, NULL, portMAX_DELAY);
            if (ret != ESP_OK)
            {
                ESP_LOGE("I2S", "Write failed: %s", esp_err_to_name(ret));
            }
        }
        else
        {
            ESP_LOGW("RINGBUF", "Incomplete frame: %d bytes", read_len);
        }
    }
}

void spi_bus_init()
{
    spi_bus_config_t buscfg = {
        .miso_io_num = MISO,
        .mosi_io_num = LCD_MOSI,
        .sclk_io_num = LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = 1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
}
void app_main()
{
    stream_buffer_create();
    ble_init();
    gps_uart_init();
    ring_buffer_create(); // 建立环形缓冲区：adc生产者，i2s/lora发送消费者
    i2s_init();           // 使用i2s接口进行音频的播放
    adc_continuous_init();

    xTaskCreate(ringbuffer_read_test, "audio_task", 8192, NULL, 8, &audio_task_handle);
    xTaskCreate(adc_continuous_read_task, "adc_read_task", 8192, NULL, 10, &s_task_handle);
    spi_bus_init();
    // // // // // 添加延时确保ADC任务已经启动 vTaskDelay(pdMS_TO_TICKS(100));
    LCD_init();
    // ExampleLLCC68Sendtask(); // 发送数据
    // // ExampleLLCC68Recivetask(); // 接收数据
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
    esp_rom_printf("finished all of the task init\n");
    while (1)
    {
        menuControl();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
