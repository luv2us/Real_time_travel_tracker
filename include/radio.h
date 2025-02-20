#ifndef _RADIO_H_
#define _RADIO_H_

// #ifdef __cplusplus
//  extern "C"
//  {
//  #endif
#include <stdint.h>
#include <stdbool.h>
#include "my_llcc68.h"
#include "llcc68_types.h"

/*!
 * \brief Radio driver definition
 */
struct Radio_s
{
    /*!
     * \brief Initializes the radio
     *
     * \param [IN] events Structure containing the driver callback functions
     */
    void (*Init)(RadioEvents_t *events);
    /*!
     * Return current radio status
     *
     * \param status Radio status.[RF_IDLE, RF_RX_RUNNING, RF_TX_RUNNING]
     */
    RadioState_t (*GetStatus)(void);
    /*!
     * \brief Configures the radio with the given modem
     *
     * \param [IN] modem Modem to be used [0: FSK, 1: LoRa]
     */
    void (*SetModem)(RadioModems_t modem);
    /*!
     * \brief Sets the channel frequency
     *
     * \param [IN] freq         Channel RF frequency
     */
    void (*SetChannel)(uint32_t freq);
    /*!
     * \brief Checks if the channel is free for the given time
     *
     * \remark The FSK modem is always used for this task as we can select the Rx bandwidth at will.
     *
     * \param [IN] freq                Channel RF frequency in Hertz
     * \param [IN] rxBandwidth         Rx bandwidth in Hertz
     * \param [IN] rssiThresh          RSSI threshold in dBm
     * \param [IN] maxCarrierSenseTime Max time in milliseconds while the RSSI is measured
     *
     * \retval isFree         [true: Channel is free, false: Channel is not free]
     */
    bool (*IsChannelFree)(uint32_t freq, uint32_t rxBandwidth, int16_t rssiThresh, uint32_t maxCarrierSenseTime);
    /*!
     * \brief Generates a 32 bits random value based on the RSSI readings
     *
     * \remark This function sets the radio in LoRa modem mode and disables
     *         all interrupts.
     *         After calling this function either Radio.SetRxConfig or
     *         Radio.SetTxConfig functions must be called.
     *
     * \retval randomValue    32 bits random value
     */
    uint32_t (*Random)(void);
    /*!
     * \brief Sets the reception parameters
     *
     * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
     * \param [IN] bandwidth    Sets the bandwidth
     *                          FSK : >= 2600 and <= 250000 Hz
     *                          LoRa: [0: 125 kHz, 1: 250 kHz,
     *                                 2: 500 kHz, 3: Reserved]
     * \param [IN] datarate     Sets the Datarate
     *                          FSK : 600..300000 bits/s
     *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
     *                                10: 1024, 11: 2048, 12: 4096  chips]
     * \param [IN] coderate     Sets the coding rate (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
     * \param [IN] bandwidthAfc Sets the AFC Bandwidth (FSK only)
     *                          FSK : >= 2600 and <= 250000 Hz
     *                          LoRa: N/A ( set to 0 )
     * \param [IN] preambleLen  Sets the Preamble length
     *                          FSK : Number of bytes
     *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
     * \param [IN] symbTimeout  Sets the RxSingle timeout value
     *                          FSK : timeout in number of bytes
     *                          LoRa: timeout in symbols
     * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
     * \param [IN] payloadLen   Sets payload length when fixed length is used
     * \param [IN] crcOn        Enables/Disables the CRC [0: OFF, 1: ON]
     * \param [IN] freqHopOn    Enables disables the intra-packet frequency hopping
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: OFF, 1: ON]
     * \param [IN] hopPeriod    Number of symbols between each hop
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: Number of symbols
     * \param [IN] iqInverted   Inverts IQ signals (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: not inverted, 1: inverted]
     * \param [IN] rxContinuous Sets the reception in continuous mode
     *                          [false: single mode, true: continuous mode]
     */
    void (*SetRxConfig)(RadioModems_t modem, uint32_t bandwidth,
                        uint32_t datarate, uint8_t coderate,
                        uint32_t bandwidthAfc, uint16_t preambleLen,
                        uint16_t symbTimeout, bool fixLen,
                        uint8_t payloadLen,
                        bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                        bool iqInverted, bool rxContinuous);
    /*!
     * \brief Sets the transmission parameters
     *
     * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
     * \param [IN] power        Sets the output power [dBm]
     * \param [IN] fdev         Sets the frequency deviation (FSK only)
     *                          FSK : [Hz]
     *                          LoRa: 0
     * \param [IN] bandwidth    Sets the bandwidth (LoRa only)
     *                          FSK : 0
     *                          LoRa: [0: 125 kHz, 1: 250 kHz,
     *                                 2: 500 kHz, 3: Reserved]
     * \param [IN] datarate     Sets the Datarate
     *                          FSK : 600..300000 bits/s
     *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
     *                                10: 1024, 11: 2048, 12: 4096  chips]
     * \param [IN] coderate     Sets the coding rate (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
     * \param [IN] preambleLen  Sets the preamble length
     *                          FSK : Number of bytes
     *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
     * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
     * \param [IN] crcOn        Enables disables the CRC [0: OFF, 1: ON]
     * \param [IN] freqHopOn    Enables disables the intra-packet frequency hopping
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: OFF, 1: ON]
     * \param [IN] hopPeriod    Number of symbols between each hop
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: Number of symbols
     * \param [IN] iqInverted   Inverts IQ signals (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [0: not inverted, 1: inverted]
     * \param [IN] timeout      Transmission timeout [ms]
     */
    void (*SetTxConfig)(RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout);
    /*!
     * \brief Checks if the given RF frequency is supported by the hardware
     *
     * \param [IN] frequency RF frequency to be checked
     * \retval isSupported [true: supported, false: unsupported]
     */
    bool (*CheckRfFrequency)(uint32_t frequency);
    /*!
     * \brief Computes the packet time on air in ms for the given payload
     *
     * \Remark Can only be called once SetRxConfig or SetTxConfig have been called
     *
     * \param [IN] modem      Radio modem to be used [0: FSK, 1: LoRa]
     * \param [IN] bandwidth    Sets the bandwidth
     *                          FSK : >= 2600 and <= 250000 Hz
     *                          LoRa: [0: 125 kHz, 1: 250 kHz,
     *                                 2: 500 kHz, 3: Reserved]
     * \param [IN] datarate     Sets the Datarate
     *                          FSK : 600..300000 bits/s
     *                          LoRa: [6: 64, 7: 128, 8: 256, 9: 512,
     *                                10: 1024, 11: 2048, 12: 4096  chips]
     * \param [IN] coderate     Sets the coding rate (LoRa only)
     *                          FSK : N/A ( set to 0 )
     *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
     * \param [IN] preambleLen  Sets the Preamble length
     *                          FSK : Number of bytes
     *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
     * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
     * \param [IN] payloadLen   Sets payload length when fixed length is used
     * \param [IN] crcOn        Enables/Disables the CRC [0: OFF, 1: ON]
     *
     * \retval airTime        Computed airTime (ms) for the given packet payload length
     */
    uint32_t (*TimeOnAir)(RadioModems_t modem, uint32_t bandwidth,
                          uint32_t datarate, uint8_t coderate,
                          uint16_t preambleLen, bool fixLen, uint8_t payloadLen,
                          bool crcOn);
    /*!
     * \brief Sends the buffer of size. Prepares the packet to be sent and sets
     *        the radio in transmission
     *
     * \param [IN]: buffer     Buffer pointer
     * \param [IN]: size       Buffer size
     */
    void (*Send)(uint8_t *buffer, uint8_t size);
    /*!
     * \brief Sets the radio in sleep mode
     */
    void (*Sleep)(void);
    /*!
     * \brief Sets the radio in standby mode
     */
    void (*Standby)(void);
    /*!
     * \brief Sets the radio in reception mode for the given time
     * \param [IN] timeout Reception timeout [ms]
     *                     [0: continuous, others timeout]
     */
    void (*Rx)(uint32_t timeout);
    /*!
     * \brief Start a Channel Activity Detection
     */
    void (*StartCad)(void);
    /*!
     * \brief Sets the radio in continuous wave transmission mode
     *
     * \param [IN]: freq       Channel RF frequency
     * \param [IN]: power      Sets the output power [dBm]
     * \param [IN]: time       Transmission mode timeout [s]
     */
    void (*SetTxContinuousWave)(uint32_t freq, int8_t power, uint16_t time);
    /*!
     * \brief Reads the current RSSI value
     *
     * \retval rssiValue Current RSSI value in [dBm]
     */
    int16_t (*Rssi)(RadioModems_t modem);
    /*!
     * \brief Writes the radio register at the specified address
     *
     * \param [IN]: addr Register address
     * \param [IN]: data New register value
     */
    void (*Write)(uint32_t addr, uint8_t data);
    /*!
     * \brief Reads the radio register at the specified address
     *
     * \param [IN]: addr Register address
     * \retval data Register value
     */
    uint8_t (*Read)(uint32_t addr);
    /*!
     * \brief Writes multiple radio registers starting at address
     *
     * \param [IN] addr   First Radio register address
     * \param [IN] buffer Buffer containing the new register's values
     * \param [IN] size   Number of registers to be written
     */
    void (*WriteBuffer)(uint32_t addr, uint8_t *buffer, uint8_t size);
    /*!
     * \brief Reads multiple radio registers starting at address
     *
     * \param [IN] addr First Radio register address
     * \param [OUT] buffer Buffer where to copy the registers data
     * \param [IN] size Number of registers to be read
     */
    void (*ReadBuffer)(uint32_t addr, uint8_t *buffer, uint8_t size);
    /*!
     * \brief Sets the maximum payload length.
     *
     * \param [IN] modem      Radio modem to be used [0: FSK, 1: LoRa]
     * \param [IN] max        Maximum payload length in bytes
     */
    void (*SetMaxPayloadLength)(RadioModems_t modem, uint8_t max);
    /*!
     * \brief Sets the network to public or private. Updates the sync byte.
     *
     * \remark Applies to LoRa modem only
     *
     * \param [IN] enable if true, it enables a public network
     */
    void (*SetPublicNetwork)(bool enable);
    /*!
     * \brief Gets the time required for the board plus radio to get out of sleep.[ms]
     *
     * \retval time Radio plus board wakeup time in ms.
     */
    uint32_t (*GetWakeupTime)(void);
    /*!
     * \brief Process radio irq
     */
    void (*IrqProcess)(void);
    /*
     * The next functions are available only on LLCC68 radios.
     */
    /*!
     * \brief Sets the radio in reception mode with Max LNA gain for the given time
     *
     * \remark Available on LLCC68 radios only.
     *
     * \param [IN] timeout Reception timeout [ms]
     *                     [0: continuous, others timeout]
     */
    void (*RxBoosted)(uint32_t timeout);
    /*!
     * \brief Sets the Rx duty cycle management parameters
     *
     * \remark Available on LLCC68 radios only.
     *
     * \param [in]  rxTime        Structure describing reception timeout value
     * \param [in]  sleepTime     Structure describing sleep timeout value
     */
    void (*SetRxDutyCycle)(uint32_t rxTime, uint32_t sleepTime);
};

/*!
 * \brief Gets the current Radio OperationMode variable
 *
 * \retval      RadioOperatingModes_t last operating mode
 */
RadioOperatingModes_t LLCC68GetOperatingMode(void);

/*!
 * \brief Sets/Updates the current Radio OperationMode variable.
 *
 * \remark WARNING: This function is only required to reflect the current radio
 *                  operating mode when processing interrupts.
 *
 * \param [in] mode           New operating mode
 */
void LLCC68SetOperatingMode(RadioOperatingModes_t mode);

/*!
 * \brief Initializes the TCXO power pin.
 */
void LLCC68IoTcxoInit(void);

/*!
 * \brief Gets the Defines the time required for the TCXO to wakeup [ms].
 *
 * \retval time Board TCXO wakeup time in ms.
 */
uint32_t LLCC68GetBoardTcxoWakeupTime(void);

/*!
 * \brief Initializes RF switch control pins.
 */
void LLCC68IoRfSwitchInit(void);

/*!
 * \brief Initializes the RF Switch I/Os pins interface
 */
void LLCC68AntSwOn(void);

/*!
 * \brief De-initializes the RF Switch I/Os pins interface
 *
 * \remark Needed to decrease the power consumption in MCU low power modes
 */
void LLCC68AntSwOff(void);

/*!
 * \brief Gets the device ID
 *
 * \retval id Connected device ID
 */
uint8_t LLCC68GetDeviceId(void);

/*!
 * \brief Sets the radio output power.
 *
 * \param [IN] power Sets the RF output power
 */
void LLCC68SetRfTxPower(int8_t power);

/*!
 * \brief Write a single byte of data to the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 * \param [in]  value         The data to be written in radio's memory
 */
void LLCC68WriteRegister(uint16_t address, uint8_t value);

/*!
 * \brief Read a single byte of data from the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 *
 * \retval      value         The value of the byte at the given address in radio's memory
 */
uint8_t LLCC68ReadRegister(uint16_t address);

/*!
 * \brief Wakes up the radio
 */
void LLCC68Wakeup(void);

/*!
 * \brief Send a command that write data to the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [in]  buffer        Buffer to be send to the radio
 * \param [in]  size          Size of the buffer to send
 */
void LLCC68WriteCommand(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);

/*!
 * \brief Send a command that read data from the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [out] buffer        Buffer holding data from the radio
 * \param [in]  size          Size of the buffer
 *
 * \retval status Return command radio status
 */
uint8_t LLCC68ReadCommand(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);

/*!
 * \brief Tx timeout timer callback
 */
void RadioOnTxTimeoutIrq(void *context);

/*!
 * \brief Rx timeout timer callback
 */
void RadioOnRxTimeoutIrq(void *context);

/*!
 * \brief Radio driver
 *
 * \remark This variable is defined and initialized in the specific radio
 *         board implementation
 */
extern const struct Radio_s Radio;
extern uint8_t MaxPayloadLength;
// #ifdef __cplusplus
// }
// #endif

#endif // __LLCC68_H__