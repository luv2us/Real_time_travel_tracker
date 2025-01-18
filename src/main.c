
#include "my_audio.h"
#include "my_adc.h"
#include "ring_buffer.h"
size_t bytes_written = 0;

int16_t read_from_ringbuffer[10240];
void ringbuffer_read_test(void *pvParameters)
{
    esp_rom_printf("Debug Ring Buffer Task Started\n");
    // int16_t debug_buffer[256];
    // int print_count = 0;

    // vTaskDelay(pdMS_TO_TICKS(1000)); // ????

    while (1)
    {
        size_t read_len = ring_buffer_read(read_from_ringbuffer, sizeof(read_from_ringbuffer));
        if (read_len > 0) //&& print_count++ < 10
        {                 // ??????
                          // printf("\nRing Buffer Data (%d samples):\n", read_len);
            // for (int i = 0; i < read_len && i < 64; i++)
            // { // ??????
            //     printf("%d,", read_from_ringbuffer[i]);
            //     if ((i + 1) % 8 == 0)
            //         printf("\n");
            // }
            i2s_channel_write(tx_chan, read_from_ringbuffer, sizeof(read_from_ringbuffer), &bytes_written, 0);
        }
        else
        {
            printf("read_len = %d\n", read_len);
            // printf("end printf\n");
        }
        vTaskDelay(pdMS_TO_TICKS(50)); // ??????
    }
}

void app_main()
{

    ring_buffer_create();
    i2s_init();
    adc_continuous_init();
    // xTaskCreate(i2s_send_audio, "sin_wave", 4096, NULL, 6, NULL);//????pcm5100

    xTaskCreate(adc_continuous_read_task, "adc_read_task", 4096, NULL, 5, &s_task_handle);
    xTaskCreate(ringbuffer_read_test, "debug_buffer", 4096, NULL, 1, NULL);
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle)); //
    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(10000)); // ??? 1s
    }
}
