
#include "my_audio.h"
#include "my_adc.h"
#include "ring_buffer.h"

int16_t record_audio_0[10080];
int16_t record_audio_1[10080];
int16_t record_audio_2[10080];
i2s_chan_handle_t tx_chan;
TaskHandle_t audio_task_handle = NULL;
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
        }};

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
    int16_t buffer[I2S_BUFFER_SIZE];
    size_t bytes_written = 0;

    // Initialize stream buffer

    while (1)
    {
        // Read audio data from stream buffer
        size_t samples_read = stream_buffer_read(buffer, I2S_BUFFER_SIZE);

        if (samples_read > 0)
        {
            // Write to I2S
            ESP_ERROR_CHECK(i2s_channel_write(tx_chan, buffer,
                                              samples_read * sizeof(int16_t), &bytes_written, portMAX_DELAY));

            if (bytes_written != samples_read * sizeof(int16_t))
            {
                ESP_LOGE(__func__, "I2S write error: expected %d, wrote %d",
                         samples_read * sizeof(int16_t), bytes_written);
            }
        }
        else
        {
            // No data available, yield to other tasks
            taskYIELD();
        }
    }
}
