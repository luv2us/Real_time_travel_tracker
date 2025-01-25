#include "my_audio.h"
#include "my_adc.h"
#include "ring_buffer.h"
#include "my_speex.h"
#include "freertos/task.h"

size_t bytes_written = 0;

int16_t read_from_ringbuffer[2560];
int16_t frame_buffer[FRAME_SIZE] = {0};

void ringbuffer_read_test(void *pvParameters)
{
    // esp_rom_printf("Debug Ring Buffer Task Started\n");

    while (1)
    {
        // ?????
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        size_t read_len = ring_buffer_read(read_from_ringbuffer, ADC_READ_LEN);
        if (read_len > 0)
        {
            printf("read_len: %d\n", read_len);
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

void app_main()
{
    ring_buffer_create();
    i2s_init();
    adc_continuous_init();
    // Speex_Init();
    // stream_buffer_create();

    xTaskCreate(ringbuffer_read_test, "audio_task", 8192, NULL, 8, &audio_task_handle);
    xTaskCreate(adc_continuous_read_task, "adc_read_task", 8192, NULL, 10, &s_task_handle);

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
