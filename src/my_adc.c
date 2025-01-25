#include "my_adc.h"
#include <inttypes.h>
// ??????
static const char *TAG = "ADC_CONTINUOUS";

// ADC ?????????
adc_continuous_handle_t adc_handle = NULL;

// DMA ??????
uint8_t __attribute__((aligned(32))) dma_buffer[ADC_READ_LEN] = {0};
// int16_t raw_adc_value[DMA_BUFFER_SIZE / 4] = {0};
SemaphoreHandle_t adc_semaphore = NULL;
int16_t __attribute__((aligned(32))) i2s_data[1280] = {0}; // 640???? * 2??????
uint32_t ret_num = 0;                                      // ????????????????
TaskHandle_t s_task_handle = NULL;

bool pool_ovf_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    esp_rom_printf("pool_ovf_cb\r\n");
    return pdTRUE; // ???????????
}

bool IRAM_ATTR on_conversion_done(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    // BaseType_t mustYield = pdFALSE;
    // // esp_rom_printf("on_conversion_done\r\n");
    // vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    // return (mustYield == pdTRUE);

    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 检查任务句柄是否有效
    if (s_task_handle != NULL)
    {
        vTaskNotifyGiveFromISR(s_task_handle, NULL);
    }
    portYIELD_FROM_ISR();

    // 直接返回高优先级任务唤醒状态
    // return xHigherPriorityTaskWoken;
    return pdTRUE;
}

void adc_continuous_init()
{
    // ESP_LOGI(TAG, "Initializing ADC continuous mode");

    adc_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(adc_semaphore);

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = DMA_BUFFER_SIZE,
        .conv_frame_size = ADC_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));
    // ???? ADC ??????????
    adc_digi_pattern_config_t pattern = {
        .atten = ADC_ATTEN_DB_12,     // 12dB
        .channel = ADC1_CHANNEL_0,    //
        .unit = ADC_UNIT,             //
        .bit_width = ADC_BITWIDTH_12, //
    };
    adc_continuous_config_t adc_config = {
        .pattern_num = 1, // ??? 1 ????????
        .adc_pattern = &pattern,
        .sample_freq_hz = ADC_SAMPLE_FREQ_HZ,   //
        .conv_mode = ADC_CONV_MODE,             //
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2, //
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
    // ESP_LOGI(__func__, "ADC continuous mode initialized");
}
void process_adc_data()
{
    int16_t *i2s_ptr = i2s_data;
    int16_t sample = 0;
    for (int i = 0; i < ret_num; i += 4)
    {
        uint32_t raw_value = *(uint32_t *)(dma_buffer + i);
        uint16_t channel = (raw_value >> 17) & 0x7;
        if (channel == 0)
        {
            uint16_t value = raw_value & 0xFFF;
            sample = (((int16_t)value) - 700) << 1;

            *i2s_ptr++ = sample; // ??????
            *i2s_ptr++ = sample; // ??????
        }
    }
}

size_t bytes_success_preload = 0;
bool ringbuffer_write_result = true;

void adc_continuous_read_task(void *arg)
{

    // vTaskPrioritySet(NULL, 24);
    while (1)
    {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        esp_err_t ret = adc_continuous_read(adc_handle, dma_buffer, ADC_READ_LEN, &ret_num, 0);
        // if (ret == ESP_OK)
        // {
        //     // esp_rom_printf("adc read %lu bytes try to write\n", ret_num);
        process_adc_data();
        printf("ret_num: %lu\n", ret_num);
        ringbuffer_write_result = ring_buffer_write(i2s_data, ret_num);
        //     if (ringbuffer_write_result == false)
        //     {
        //         esp_rom_printf("Failed to write to ring buffer\n");
        //     }
        //     if (audio_task_handle != NULL)
        //     {
        xTaskNotifyGive(audio_task_handle);
        //     }
        // }
        // else
        // {
        //     ESP_LOGE(TAG, "adc_continuous_read failed with error: %s", esp_err_to_name(ret));
        // }
    }
}
