#include "my_adc.h"

static const char *TAG = "ADC_CONTINUOUS";

adc_continuous_handle_t adc_handle = NULL;

uint8_t __attribute__((aligned(32))) dma_buffer[ADC_READ_LEN] = {0};

int16_t __attribute__((aligned(32))) i2s_data[ADC_READ_LEN / 4] = {0};

uint32_t ret_num = 0;

TaskHandle_t s_task_handle = NULL;

int battery_voltage = 0;

adc_cali_handle_t adc_cali_handle = NULL;
float alpha = 0.1; // 滤波系数，范围为 0 ~ 1，值越小滤波效果越强
float filtered_value = 0;

void low_pass_filter(int16_t raw_value)
{
    filtered_value = alpha * raw_value + (1 - alpha) * filtered_value;
}
bool pool_ovf_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    esp_rom_printf("pool_ovf_cb\r\n");
    return pdTRUE;
}

bool IRAM_ATTR on_conversion_done(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    if (s_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(s_task_handle, NULL);
    }
    else
    {
        esp_rom_printf("s_task_handle is NULL\r\n");
    }
    portYIELD_FROM_ISR();
    return pdTRUE;
}

void adc_continuous_init()
{
    // s_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = DMA_BUFFER_SIZE,
        .conv_frame_size = ADC_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern[2] = {
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = ADC1_CHANNEL_0,
            .unit = ADC_UNIT_1,
            .bit_width = ADC_BITWIDTH_12,
        },
        {
            .atten = ADC_ATTEN_DB_12,
            .channel = ADC1_CHANNEL_3,
            .unit = ADC_UNIT_1,
            .bit_width = ADC_BITWIDTH_12,
        }};

    adc_continuous_config_t adc_config = {
        .pattern_num = 2,
        .adc_pattern = pattern,
        .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = on_conversion_done,
        .on_pool_ovf = pool_ovf_cb,
    };
    esp_err_t ret = adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE("ADC_CONTINUOUS", "Failed to register callbacks, error: %s", esp_err_to_name(ret));
    }

    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_config));
    ESP_LOGI(__func__, "ADC continuous mode initialized");

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_WIDTH_BIT_12,
        .chan = ADC_CHANNEL_4,
    };

    adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
}

// int16_t i2s_data[2 * ADC_READ_LEN];

int process_adc_data()
{
    int16_t *i2s_ptr = i2s_data;
    static int voltage_sample_counter = (ADC_SAMPLE_FREQ_HZ * 30);
    int i = 0;

    int channel0_count = 0;
    // int channel3_count = 0;
    for (i = 0; i < ret_num / sizeof(adc_digi_output_data_t); i++)
    {

        // uint32_t raw_value = *(uint32_t *)(dma_buffer + i * sizeof(uint32_t));
        // uint16_t channel = (raw_value >> 13) & 0x7;
        // uint16_t value = raw_value & 0xFFF;
        adc_digi_output_data_t *raw_value = (adc_digi_output_data_t *)(dma_buffer + i * sizeof(adc_digi_output_data_t));
        uint16_t channel = (raw_value->type2.channel);
        uint16_t value = (raw_value->type2.data);
        if (channel == ADC1_CHANNEL_0)
        {
            low_pass_filter(value);
            int16_t audio_value = (((int16_t)value) - 700) << 1;
            // i2s_data[channel0_count++] = audio_value;
            // i2s_data[channel0_count++] = audio_value;
            *i2s_ptr++ = audio_value;
            *i2s_ptr++ = audio_value;
            channel0_count++;
        }
        else if (channel == ADC1_CHANNEL_3)
        {
            voltage_sample_counter++;
            if (voltage_sample_counter >= (ADC_SAMPLE_FREQ_HZ * 30)) // 30秒采样
            {
                adc_cali_raw_to_voltage(adc_cali_handle, value, &battery_voltage);
                voltage_sample_counter = 0;
            }
            // channel3_count++;
        }
    }
    // esp_rom_printf("channel0 count: %d\r\n", channel0_count);
    // esp_rom_printf("channel3 count: %d\r\n", channel3_count);
    return channel0_count;
}

size_t bytes_success_preload = 0;
bool ringbuffer_write_result = true;

// ADC数据读取任务
void adc_continuous_read_task(void *arg)
{
    vTaskPrioritySet(NULL, 24);

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_err_t ret = adc_continuous_read(adc_handle, dma_buffer, ADC_READ_LEN, &ret_num, 0);

        if (ret == ESP_OK)
        {

            process_adc_data();
            // esp_rom_printf("Read  %d samples", audio_sample_num);
            ring_buffer_write(i2s_data, (ret_num / 2));

            if (audio_task_handle != NULL)
            {
                xTaskNotifyGive(audio_task_handle);
            }
            else
            {
                ESP_LOGE(TAG, "Attempted to notify invalid audio task handle");
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read ADC data: %d", ret);
        }
    }
}
