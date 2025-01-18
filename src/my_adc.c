#include "my_adc.h"
#include <inttypes.h>
// ??????
static const char *TAG = "ADC_CONTINUOUS";

// ADC ?????????
adc_continuous_handle_t adc_handle = NULL;

// DMA ??????
uint8_t dma_buffer[DMA_BUFFER_SIZE] = {0};
int16_t raw_adc_value[DMA_BUFFER_SIZE / 4] = {0};
SemaphoreHandle_t adc_semaphore = NULL;
int16_t i2s_data[DMA_BUFFER_SIZE * 2] = {0};
uint32_t ret_num = 0; // ????????????????
TaskHandle_t s_task_handle = NULL;
bool pool_ovf_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    esp_rom_printf("pool_ovf_cb\r\n");
    // adc_continuous_read(adc_handle, dma_buffer, DMA_BUFFER_SIZE, &ret_num, 0);
    // ESP_LOGW(TAG, "Pool overflow");
    /* copilot
     BaseType_t high_task_awoken = pdFALSE;

    // ???????????????
    uint32_t ret_num = 0;
    esp_err_t ret = adc_continuous_read(handle, dma_buffer, DMA_BUFFER_SIZE, &ret_num, 0);

    if (ret == ESP_OK && ret_num > 0)
    {
        // ????ADC?????????????????
        int16_t temp_buffer[256];
        int temp_index = 0;

        for (int i = 0; i < ret_num; i += 4)
        {
            uint32_t raw_value = *(uint32_t *)(dma_buffer + i);
            uint16_t value = raw_value & 0xFFF;
            temp_buffer[temp_index++] = (((int16_t)value) - 2048) << 4;

            if (temp_index >= 256)
            {
                ring_buffer_write(temp_buffer, temp_index);
                temp_index = 0;
                return high_task_awoken == pdTRUE;
    }
}
    }

    portYIELD_FROM_ISR(high_task_awoken);
    high_task_awoken == */
    return pdTRUE; // ???????????
}

bool IRAM_ATTR on_conversion_done(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    esp_rom_printf("on_conversion_done\r\n");

    // Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

    return (mustYield == pdTRUE);

    // BaseType_t high_task_awoken = pdFALSE;
    // // ??????ADC????
    // int16_t temp_buffer[ADC_READ_LEN / 4];
    // uint8_t *conv_data = edata->conv_frame_buffer;
    // uint32_t size = edata->size;

    // int temp_index = 0;
    // for (int i = 0; i < size; i += 4)
    // {
    //     uint32_t raw_value = *(uint32_t *)(conv_data + i);
    //     uint16_t value = raw_value & 0xFFF;
    //     temp_buffer[temp_index++] = (((int16_t)value) - 2048) << 4;
    // }
    // // ????????????
    // if (temp_index > 0)
    // {
    //     ring_buffer_write(temp_buffer, temp_index);
    // }
    // // adc_continuous_read(adc_handle, temp_buffer, ADC_READ_LEN, &ret_num, 0);
    // //  ??????????????
    // //  ESP_LOGI("ADC_CONTINUOUS", "Conversion done, sample: %d", edata->sample);
    // //  ???? edata->buffer ????????
    // portYIELD_FROM_ISR(high_task_awoken);
    // return high_task_awoken == pdTRUE; // ???????????
}
// ????? ADC ??????

void adc_continuous_init()
{
    ESP_LOGI(TAG, "Initializing ADC continuous mode");
    // ??????????????
    adc_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(adc_semaphore);
    // ???? ADC ?????????
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = DMA_BUFFER_SIZE, // ???????????????
        .conv_frame_size = ADC_READ_LEN,       // ?????????????
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));
    // ???? ADC ??????????
    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_DB_12,     // 12dB ???
        .channel = ADC1_CHANNEL_0,    // ???????
        .unit = ADC_UNIT,             // ADC ???
        .bit_width = ADC_BITWIDTH_12, // 12 ??????????
    };
    adc_continuous_config_t adc_config = {
        .pattern_num = 1, // ??? 1 ????????
        .adc_pattern = &pattern,
        .sample_freq_hz = ADC_SAMPLE_FREQ_HZ,   // ???????
        .conv_mode = ADC_CONV_MODE,             // ?????
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2, // ??? type2 ???
    };
    // ?????????
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = on_conversion_done,
        .on_pool_ovf = pool_ovf_cb,
    };
    esp_err_t ret = adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE("ADC_CONTINUOUS", "Failed to register callbacks, error: %s", esp_err_to_name(ret));
    }
    //

    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_config));
    ESP_LOGI(__func__, "ADC continuous mode initialized");
}
void process_adc_data()
{
    int16_t *i2s_ptr = i2s_data;
    for (int i = 0; i < ret_num; i += 4)
    {
        uint32_t raw_value = *(uint32_t *)(dma_buffer + i);
        uint16_t channel = (raw_value >> 17) & 0x7;
        if (channel == 0)
        {
            uint16_t value = raw_value & 0xFFF;
            int16_t sample = (((int16_t)value) - 2048);
            *i2s_ptr++ = sample; // ??????
                                 // *i2s_ptr++ = sample; // ??????
        }
    }
}
size_t bytes_success_preload = 0;
bool ringbuffer_write_result = true;
// ??? ADC ????????
void adc_continuous_read_task(void *arg)
{

    vTaskPrioritySet(NULL, 24);
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_err_t ret = adc_continuous_read(adc_handle, dma_buffer, DMA_BUFFER_SIZE, &ret_num, 0);
        if (ret == ESP_OK)
        {
            esp_rom_printf("adc read %lu bytes try to write\n", ret_num);
            process_adc_data();
            ringbuffer_write_result = ring_buffer_write(i2s_data, ret_num);
            if (ringbuffer_write_result == false)
            {
                esp_rom_printf("Failed to write to ring buffer\n");
            }
            // i2s_channel_write(tx_chan, i2s_data, sizeof(i2s_data), &bytes_written, 0);
        }
        else
        {
            ESP_LOGE(TAG, "adc_continuous_read failed");
        }
    }
}
