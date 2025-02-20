#ifndef _LLCC68_BOARD_H_
#define _LLCC68_BOARD_H_
// #ifdef __cplusplus
// extern "C"
// {
// #endif
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "my_llcc68.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#define SOFT_VERSION "LLCC68 driver for stm32f103 V0.0.0"

//--------------------------------------------- ??????????? ---------------------------------------------

#define LORA_FRE 470000000           // LoRa ????? 868.5 MHz
#define LORA_TX_OUTPUT_POWER 22      // LoRa ??????? 20 dBm
#define LORA_BANDWIDTH 0             // LoRa ????? 250 kHz
                                     // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 9      // LoRa ??????? 9
                                     // ??????? 7 ? 12
#define LORA_CODINGRATE 1            // LoRa ?????? 4/5
                                     // ????? [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH 8       // LoRa ???????? 8 ??
#define LORA_LLCC68_SYMBOL_TIMEOUT 0 // LoRa ????????? 0
                                     // ????????? 0??????? 127?5 ?????
#define LORA_FIX_LENGTH_PAYLOAD_ON 0 // LoRa ????????????
                                     // ????????? (LLCC68 ??)
#define LORA_IQ_INVERSION_ON false   // LoRa IQ ???????
                                     // IQ ???? (LLCC68 ??)
#define LORA_RX_TIMEOUT_VALUE 5000   // LoRa ????????? 5000 ??

/*!
 * Board MCU pins definitions
 */
// SPI
#define RADIO_NSS_PIN GPIO_NUM_10

#define RADIO_MOSI_PIN 11

#define RADIO_MISO_PIN 13

#define RADIO_SCK_PIN 12

// RST??¦Ë??
#define RADIO_nRESET_PIN GPIO_NUM_47

// DIO1 ????
#define RADIO_DIO1_PIN GPIO_NUM_21

// BUSY ????
#define RADIO_DIO4_BUSY_PIN GPIO_NUM_14

// // TXEN
// #define RADIO_DIO0_TXEN_PIN GPIO_Pin_10
// #define RADIO_DIO0_TXEN_PORT GPIOB
// // DIO2
// #define RADIO_DIO2_PIN GPIO_Pin_8
// #define RADIO_DIO2_PORT GPIOB
// // DIO3
// #define RADIO_DIO3_PIN GPIO_Pin_9
// #define RADIO_DIO3_PORT GPIOB
// // RXEN
// #define RADIO_DIO5_RXEN_PIN GPIO_Pin_1
// #define RADIO_DIO5_RXEN_PORT GPIOA
static portMUX_TYPE LLCC68CriticalSectionMutex = portMUX_INITIALIZER_UNLOCKED;
;

#define HAVE_OS 1
// ?????????????????????
#if HAVE_OS
#define CRITICAL_SECTION_BEGIN() portENTER_CRITICAL(&LLCC68CriticalSectionMutex)
#define CRITICAL_SECTION_END() vPortExitCritical(&LLCC68CriticalSectionMutex)
#else
#define CRITICAL_SECTION_BEGIN()
#define CRITICAL_SECTION_END()
#endif
void LLCC68DelayMs(uint32_t ms);
/*!
 * \brief Initializes the radio I/Os pins interface
 */
void LLCC68IoInit(void);

/*!
 * \brief Initializes DIO IRQ handlers
 *
 * \param [IN] irqHandlers Array containing the IRQ callback functions
 */
void LLCC68IoIrqInit(DioIrqHandler dioIrq);

/*!
 * \brief De-initializes the radio I/Os pins interface.
 *
 * \remark Useful when going in MCU low power modes
 */
void LLCC68IoDeInit(void);

/*!
 * \brief Initializes the radio debug pins.
 */
void LLCC68IoDbgInit(void);

/*!
 * \brief HW Reset of the radio
 */
void LLCC68Reset(void);

/*!
 * \brief Blocking loop to wait while the Busy pin in high
 */
void LLCC68WaitOnBusy(void);

/*!
 * \brief Checks if the given RF frequency is supported by the hardware
 *
 * \param [IN] frequency RF frequency to be checked
 * \retval isSupported [true: supported, false: unsupported]
 */
bool LLCC68CheckRfFrequency(uint32_t frequency);

void LLCC68DelayMs(uint32_t ms);
void dio1IraWrapper();
/*
 * ??????????
 */
void LLCC68TimerInit(void);
void LLCC68SetTxTimerValue(uint32_t nMs);
void LLCC68TxTimerStart(void);
void LLCC68TxTimerStop(void);
void LLCC68SetRxTimerValue(uint32_t nMs);
void LLCC68RxTimerStart(void);
void LLCC68RxTimerStop(void);

void LLCC68SetNss(uint8_t lev);
uint8_t LLCC68SpiInOut(uint8_t data);

/*!
 * Radio hardware and global parameters
 */
extern LLCC68_t LLCC68;
// #ifdef __cplusplus
// }
// #endif
#endif