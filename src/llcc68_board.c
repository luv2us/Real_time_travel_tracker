#include "llcc68_board.h"
#include "radio.h"
#include <stdio.h>

typedef struct __softTimer_s
{
    uint8_t isRun;    // 0????1????
    uint32_t delayMs; // ????????????????
    uint32_t startMs; // ???????????
} SoftTimer_t;
// static DioIrqHandler *dio1IrqCallback = NULL; // ???DIO1???????????
static uint32_t timer_count = 0;
spi_device_handle_t lora;
void LLCC68IoInit(void)
{
    // uint8_t u8_ret = 255;
    gpio_config_t lora_io_cfg = {0};
    lora_io_cfg.pin_bit_mask = 1ULL << RADIO_DIO4_BUSY_PIN;
    lora_io_cfg.mode = GPIO_MODE_INPUT;
    lora_io_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    lora_io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    lora_io_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&lora_io_cfg);
    lora_io_cfg.pin_bit_mask = 1ULL << RADIO_NSS_PIN;
    lora_io_cfg.mode = GPIO_MODE_OUTPUT;
    lora_io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&lora_io_cfg);
    spi_bus_config_t buscfg = {
        .miso_io_num = RADIO_MISO_PIN,
        .mosi_io_num = RADIO_MOSI_PIN,
        .sclk_io_num = RADIO_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = 1,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // SPI??????
        .mode = 0,                             // SPI??0
        .spics_io_num = -1,                    // ?????CS????
        .queue_size = 2,                       // ???????
    };
    // ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &lora));
    printf("spi io init\n");
}
// void dio1IraWrapper()
// {
//     if (dio1IrqCallback != NULL)
//     {
//         dio1IrqCallback(NULL); // ???? context ? NULL
//     }
//     else
//     {
//         esp_rom_printf("dio1IrqCallback null\n");
//     }
// }
void gpio_isr_handler(void *parameter)
{
}
void LLCC68IoIrqInit(gpio_isr_t isr_handle)
{
    gpio_config_t lora_dio1 = {0};
    lora_dio1.pin_bit_mask = 1ULL << RADIO_DIO1_PIN;
    lora_dio1.mode = GPIO_MODE_INPUT;
    lora_dio1.pull_up_en = GPIO_PULLUP_DISABLE;
    lora_dio1.pull_down_en = GPIO_PULLDOWN_ENABLE;
    lora_dio1.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&lora_dio1);
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(RADIO_DIO1_PIN, isr_handle, NULL);
    printf("dio1 irq init\n");
}
void LLCC68IoDeInit(void)
{
}
void LLCC68Reset(void)
{
    LLCC68DelayMs(10);
    gpio_config_t nRESET = {0};
    nRESET.pin_bit_mask = 1ULL << RADIO_nRESET_PIN;
    nRESET.mode = GPIO_MODE_OUTPUT;
    gpio_config(&nRESET);
    gpio_set_level(RADIO_nRESET_PIN, 0);
    LLCC68DelayMs(20);
    gpio_set_direction(RADIO_nRESET_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(RADIO_nRESET_PIN, GPIO_PULLUP_ONLY);

    LLCC68DelayMs(10);
}
void LLCC68WaitOnBusy(void)
{
    uint32_t u32_count = 0;

    while (gpio_get_level(RADIO_DIO4_BUSY_PIN) == 1)
    {
        if (u32_count++ > 1000)
        {
            printf("wait busy pin timeout\r\n");
            u32_count = 0;
        }
        LLCC68DelayMs(1);
    }
}
void LLCC68SetNss(uint8_t lev)
{
    if (lev)
    {
        gpio_set_level(RADIO_NSS_PIN, 1); // ???????
    }
    else
    {
        gpio_set_level(RADIO_NSS_PIN, 0); // ???????
    }
}

uint8_t LLCC68SpiInOut(uint8_t data)
{
    uint8_t tx_data = data;
    uint8_t rx_data = 0;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &tx_data,
        .rx_buffer = &rx_data};
    esp_err_t ret = spi_device_polling_transmit(lora, &t);
    if (ret != ESP_OK)
    {
        printf("SPI transmit error: %s\n", esp_err_to_name(ret));
        return 0;
    }
    return rx_data;
}
bool LLCC68CheckRfFrequency(uint32_t frequency)
{
    // Implement check. Currently all frequencies are supported
    return true;
}
void LLCC68DelayMs(uint32_t ms)
{
    vTaskDelay(ms);
}
static SoftTimer_t txTimerHandle, rxTimerHandle;
void LLCC68TxTimerIrq(TimerHandle_t xTimer)
{
    uint32_t diffMs = 0;
    timer_count++;
    if (txTimerHandle.isRun)
    {
        if (timer_count > txTimerHandle.startMs)
        {
            diffMs = xTaskGetTickCount() - txTimerHandle.startMs;
        }
        else
        {
            diffMs = 0xffffffff - txTimerHandle.startMs + timer_count;
        }
        if (diffMs >= txTimerHandle.delayMs)
        {
            LLCC68TxTimerStop();
            txTimerHandle.startMs = timer_count;
            // printf("[%s()-%d]freertos timeout\r\n", __func__, __LINE__);

            RadioOnTxTimeoutIrq(NULL); // tx??????
        }
    }
}

void LLCC68RxTimerIrq(TimerHandle_t xTimer)
{
    uint32_t diffMs = 0;
    timer_count++;
    if (rxTimerHandle.isRun)
    {
        if (timer_count > rxTimerHandle.startMs)
        {
            diffMs = xTaskGetTickCount() - rxTimerHandle.startMs;
        }
        else
        {
            diffMs = 0xffffffff - rxTimerHandle.startMs + timer_count;
        }
        if (diffMs >= rxTimerHandle.delayMs)
        {
            LLCC68RxTimerStop();
            rxTimerHandle.startMs = timer_count;
            RadioOnRxTimeoutIrq(NULL); // rx??????
        }
    }
}
void LLCC68TimerIrq(TimerHandle_t xTimer)
{
    uint32_t diffMs = 0;
    timer_count++;
    if (txTimerHandle.isRun)
    {
        if (timer_count > txTimerHandle.startMs)
        {
            diffMs = xTaskGetTickCount() - txTimerHandle.startMs;
        }
        else
        {
            diffMs = 0xffffffff - txTimerHandle.startMs + timer_count;
        }
        if (diffMs >= txTimerHandle.delayMs)
        {
            LLCC68TxTimerStop();
            txTimerHandle.startMs = timer_count;
            // printf("[%s()-%d]freertos timeout\r\n", __func__, __LINE__);

            RadioOnTxTimeoutIrq(NULL); // tx??????
        }
    }
    if (rxTimerHandle.isRun)
    {
        if (timer_count > rxTimerHandle.startMs)
        {
            diffMs = xTaskGetTickCount() - rxTimerHandle.startMs;
        }
        else
        {
            diffMs = 0xffffffff - rxTimerHandle.startMs + timer_count;
        }
        if (diffMs >= rxTimerHandle.delayMs)
        {
            LLCC68RxTimerStop();
            rxTimerHandle.startMs = timer_count;
            RadioOnRxTimeoutIrq(NULL); // rx??????
        }
    }
}
// static TimerHandle_t txTimer;
// static TimerHandle_t rxTimer;
static TimerHandle_t LoRaTimer;
void LLCC68TimerInit(void)
{
    printf("Initializing LoRa timer...\n");
    LoRaTimer = xTimerCreate("LoRaTimer", 1, pdTRUE, (void *)0, LLCC68TimerIrq);
    if (LoRaTimer == NULL)
    {
        printf("Failed to create LoRa timer - check FreeRTOS heap size\n");
        return;
    }
    txTimerHandle.isRun = 0;
    rxTimerHandle.isRun = 0;
    if (xTimerStart(LoRaTimer, 0) != pdPASS)
    {
        printf("Failed to start LoRa timer\n");
        return;
    }
    printf("LoRa timer initialized successfully\n");
}

void LLCC68SetTxTimerValue(uint32_t nMs)
{
    esp_rom_printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
    txTimerHandle.delayMs = nMs;
}

void LLCC68TxTimerStart(void)
{
    esp_rom_printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
    txTimerHandle.startMs = timer_count;
    txTimerHandle.isRun = 1;
    // xTimerStart(txTimer, 0);
}

void LLCC68TxTimerStop(void)
{
    esp_rom_printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
    txTimerHandle.isRun = 0;
    // xTimerStop(txTimer, 0);
}

void LLCC68SetRxTimerValue(uint32_t nMs)
{
    esp_rom_printf("[%s()-%d]set timer out %d ms\r\n", __func__, __LINE__, nMs);
    rxTimerHandle.delayMs = nMs;
}

void LLCC68RxTimerStart(void)
{
    esp_rom_printf("[%s()-%d]start timer\r\n", __func__, __LINE__);
    rxTimerHandle.startMs = timer_count;
    rxTimerHandle.isRun = 1;
    // xTimerStart(rxTimer, 0);
}

void LLCC68RxTimerStop(void)
{
    esp_rom_printf("[%s()-%d]stop timer\r\n", __func__, __LINE__);
    rxTimerHandle.isRun = 0;
    //  xTimerStop(rxTimer, 0);
}
