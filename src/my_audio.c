/*// #include "my_audio.h"
// #include "my_adc.h"
// int16_t record_audio_0[10080];
// int16_t record_audio_1[10080];
// int16_t record_audio_2[10080];
// void i2s_init()
// {
//     // I2S ?????
//     i2s_config_t i2s_config = {
//         .mode = I2S_MODE_MASTER | I2S_MODE_TX,             // ??????????
//         .sample_rate = SAMPLE_RATE,                        // ??????
//         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,      // 16 ??????
//         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,      // ??????
//         .communication_format = I2S_COMM_FORMAT_STAND_I2S, // ??? I2S ???
//         .dma_buf_count = 10,                               // DMA ??????????
//         .dma_buf_len = 1024,                               // ?????????????
//         .use_apll = false,                                 // ????? APLL
//         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1           // ?????????
//     };

//     // I2S ????????
//     i2s_pin_config_t pin_config = {
//         .bck_io_num = I2S_BCK_PIN,       // ?????
//         .ws_io_num = I2S_WS_PIN,         // ?????
//         .data_out_num = I2S_DOUT_PIN,    // ???????
//         .data_in_num = I2S_PIN_NO_CHANGE // ?????????
//     };

//     // ??? I2S ????
//     i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
//     i2s_set_pin(I2S_NUM, &pin_config);
//     gpio_config_t io_conf = {};
//     io_conf.intr_type = GPIO_INTR_DISABLE;            // ????????
//     io_conf.mode = GPIO_MODE_OUTPUT;                  // ??????????
//     io_conf.pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN); // ??????????????
//     io_conf.pull_down_en = 0;                         // ????????
//     io_conf.pull_up_en = 0;                           // ????????
//     gpio_config(&io_conf);

//     // ?? GPIO ????????????
//     gpio_set_level(GPIO_OUTPUT_PIN, 1);
//     ESP_LOGI(__func__, "I2S initialized");
// }
// void generate_stereo_sine_wave(int16_t *buffer, size_t size, float frequency)
// {
//     float phase_increment = 2 * PI * frequency / SAMPLE_RATE;
//     float phase = 0;

//     for (int i = 0; i < size / 2; i++)
//     {
//         // ?????????
//         int16_t sample = (int16_t)(AMPLITUDE * sin(phase)); // ?????
//         buffer[2 * i] = sample;                             // ???
//         buffer[2 * i + 1] = sample;                         // ???
//         phase += phase_increment;
//         if (phase >= 2 * PI)
//         {
//             phase -= 2 * PI; // ????
//         }
//     }
// }
// void i2s_send_audio()
// {
//     int16_t *buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
//     if (buffer == NULL)
//     {
//         ESP_LOGE(__func__, "Failed to allocate memory for buffer");
//         return;
//     }

//     // // ?? 440Hz ???
//     // generate_stereo_sine_wave(buffer, BUFFER_SIZE, 440.0);
//     for (int16_t i = 0; i < 30240; i++)
//     {
//         if (i < 10080)
//         {
//             record_audio_0[i] = record_audio[i];
//         }
//         else if (i < 20160)
//         {
//             record_audio_1[i & 10080] = record_audio[i];
//         }
//         else
//         {
//             record_audio_2[i & 20160] = record_audio[i];
//         }
//     }

//     while (1)
//     {
//         // if (xSemaphoreTake(adc_semaphore, portMAX_DELAY) == pdTRUE)
//         // {
//         size_t bytes_written = 0;

//         // ????? I2S
//         // i2s_write(I2S_NUM, record_audio_0, sizeof(record_audio_0), &bytes_written, portMAX_DELAY);
//         // if (bytes_written != sizeof(record_audio_0))
//         // {
//         //     ESP_LOGE(__func__, "I2S write0 error");
//         // }
//         // i2s_write(I2S_NUM, record_audio_1, sizeof(record_audio_1), &bytes_written, portMAX_DELAY);
//         // if (bytes_written != sizeof(record_audio_1))
//         // {
//         //     ESP_LOGE(__func__, "I2S write1 error");
//         // }
//         // i2s_write(I2S_NUM, record_audio_2, sizeof(record_audio_2), &bytes_written, portMAX_DELAY);
//         // if (bytes_written != sizeof(record_audio_2))
//         // {
//         //     ESP_LOGE(__func__, "I2S write2 error");
//         // }
//         // xSemaphoreGive(adc_semaphore);
//         // }
//         i2s_write(I2S_NUM, record_audio, sizeof(record_audio), &bytes_written, portMAX_DELAY);
//         if (bytes_written != sizeof(record_audio))
//         {
//             ESP_LOGE(__func__, "I2S write0 error");
//         }
//         vTaskDelay(pdMS_TO_TICKS(5000));
//     }

//     free(buffer);
// }*/
#include "my_audio.h"
#include "my_adc.h"

int16_t record_audio_0[10080];
int16_t record_audio_1[10080];
int16_t record_audio_2[10080];
i2s_chan_handle_t tx_chan;
void i2s_init()
{
    // ??I2S??
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DOUT_PIN,
            .din = GPIO_NUM_NC,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // ??I2S??
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    // GPIO??
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0};
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_PIN, 1);

    ESP_LOGI(__func__, "I2S initialized with new driver");
}

void generate_stereo_sine_wave(int16_t *buffer, size_t size, float frequency)
{
    float phase_increment = 2 * PI * frequency / SAMPLE_RATE;
    float phase = 0;

    for (int i = 0; i < size / 2; i++)
    {
        int16_t sample = (int16_t)(AMPLITUDE * sin(phase));
        buffer[2 * i] = sample;
        buffer[2 * i + 1] = sample;
        phase += phase_increment;
        if (phase >= 2 * PI)
        {
            phase -= 2 * PI;
        }
    }
}

void i2s_send_audio()
{
    /*   int16_t *buffer = (int16_t *)malloc(BUFFER_SIZE * sizeof(int16_t));
    if (buffer == NULL)
    {
        ESP_LOGE(__func__, "Failed to allocate memory for buffer");
        return;
    }*/

    /*    for (int16_t i = 0; i < 30240; i++)
        {
            if (i < 10080)
            {
                record_audio_0[i] = record_audio[i];
            }
            else if (i < 20160)
            {
                record_audio_1[i & 10080] = record_audio[i];
            }
            else
            {
                record_audio_2[i & 20160] = record_audio[i];
            }
        }*/

    while (1)
    {
        size_t bytes_written = 0;
        i2s_channel_write(tx_chan, record_audio, sizeof(record_audio), &bytes_written, portMAX_DELAY);
        if (bytes_written != sizeof(record_audio))
        {
            ESP_LOGE(__func__, "I2S write0 error");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    /* free(buffer);*/
}
