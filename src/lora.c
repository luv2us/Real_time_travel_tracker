#include "lora.h"
#include "my_ble.h"
#include "my_gps.h"
#include "ring_buffer.h"
static RadioEvents_t LLCC68RadioEvents;
static uint8_t rxRetryCount = 0; // ??????
TaskHandle_t lora_send_task = NULL;
TaskHandle_t lora_recv_task = NULL;
#define MAX_RX_RETRY 3
static void LLCC68OnTxDone(void);
static void LLCC68OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void LLCC68OnTxTimeout(void);
static void LLCC68OnRxTimeout(void);
static void LLCC68OnRxError(void);

void send_int16_array(int16_t *array, size_t length)
{
    const size_t max_elements = sizeof(((Packet_t *)0)->data) / sizeof(int16_t);
    const uint16_t total_packets = (length + max_elements - 1) / max_elements;

    for (uint16_t pkt_idx = 0; pkt_idx < total_packets; pkt_idx++)
    {
        Packet_t packet = {
            .total_packets = total_packets,
            .packet_index = pkt_idx};

        size_t start_idx = pkt_idx * max_elements;
        size_t elements_to_send = (length - start_idx) > max_elements ? max_elements : (length - start_idx);

        memcpy(packet.data, &array[start_idx], elements_to_send * sizeof(int16_t));

        // ????????????????????4??? + ????
        uint8_t pkt_size = 4 + elements_to_send * sizeof(int16_t);
        Radio.Send((uint8_t *)&packet, pkt_size);
    }
}
void lora_send(void *parameter)
{
    while (1)
    {
        // printf("%s", packet);
        Radio.IrqProcess(); // Process Radio IRQ

        uint16_t packetsize = strlen(packet);
        if (xSemaphoreTake(gps2lora_semaphore, portMAX_DELAY))
        {

            // send_int16_array(int16_array, array_length);
            gps_changed = false;

            //  xSemaphoreGive(gps2lora_semaphore);
        }
        // LLCC68DelayMs(1000);
    }
}
void lora_recv(void *parameter)
{
    while (1)
    {
        // printf("%s", packet);
        Radio.IrqProcess(); // Process Radio IRQ

        LLCC68DelayMs(100);
    }
}
// ?????????????????????1S???????????
void ExampleLLCC68Sendtask(void)
{

    uint8_t OCP_Value = 0;

    printf("start %s() example\r\n", __func__);
    // ?????????????????
    LLCC68RadioEvents.TxDone = LLCC68OnTxDone;
    LLCC68RadioEvents.RxDone = LLCC68OnRxDone;
    LLCC68RadioEvents.TxTimeout = LLCC68OnTxTimeout;
    LLCC68RadioEvents.RxTimeout = LLCC68OnRxTimeout;
    LLCC68RadioEvents.RxError = LLCC68OnRxError;

    Radio.Init(&LLCC68RadioEvents);
    // ?????????????
    Radio.SetChannel(LORA_FRE);
    // ???????????
    Radio.SetTxConfig(MODEM_LORA,
                      LORA_TX_OUTPUT_POWER,
                      0,
                      LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true,
                      0,
                      0,
                      LORA_IQ_INVERSION_ON,
                      3000);
    // ??? OCP ??????
    OCP_Value = Radio.Read(REG_OCP);
    printf("[%s()-%d]read OCP register value:0x%04X\r\n", __func__, __LINE__, OCP_Value);
    // ??????????
    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_LLCC68_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, false);

    printf("all setting\r\n");
    printf("freq: %d\r\n Tx power: %d\r\n band width: %d\r\n FS: %d\r\n CODINGRATE: %d\r\n PREAMBLE_LENGTH: %d\r\n", LORA_FRE, LORA_TX_OUTPUT_POWER, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH);
    xTaskCreate(lora_send, "lora_send_task", 4096, NULL, 8, &lora_send_task);
}

// ?????????????????????1S???????????
void ExampleLLCC68Recivetask(void)
{
    uint8_t OCP_Value = 0;
    // printf("start %s() example\r\n", __func__);
    // ?????????????????
    LLCC68RadioEvents.TxDone = LLCC68OnTxDone;
    LLCC68RadioEvents.RxDone = LLCC68OnRxDone;
    LLCC68RadioEvents.TxTimeout = LLCC68OnTxTimeout;
    LLCC68RadioEvents.RxTimeout = LLCC68OnRxTimeout;
    LLCC68RadioEvents.RxError = LLCC68OnRxError;

    Radio.Init(&LLCC68RadioEvents);
    // ?????????????
    Radio.SetChannel(LORA_FRE);
    // ???????????
    Radio.SetTxConfig(MODEM_LORA,
                      LORA_TX_OUTPUT_POWER,
                      0,
                      LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true,
                      0,
                      0,
                      LORA_IQ_INVERSION_ON,
                      3000); // ??????lora??,???????,fsk???lora?????0???????????????????????????????????????????(????????????????false)??crc????0???????????????????????(????????????????????)??????????????????????????????????
    // ??? OCP ??????
    OCP_Value = Radio.Read(REG_OCP);
    printf("[%s()-%d]read OCP register value:0x%04X\r\n", __func__, __LINE__, OCP_Value);
    // ??????????
    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_LLCC68_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

    printf("start enter rx mode\r\n");
    Radio.Rx(0); // ?????????
    printf("all setting\r\n");
    printf("freq: %d\r\n Tx power: %d\r\n band width: %d\r\n FS: %d\r\n CODINGRATE: %d\r\n PREAMBLE_LENGTH: %d\r\n", LORA_FRE, LORA_TX_OUTPUT_POWER, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH);
    xTaskCreate(lora_recv, "lora_recv_task", 4096, NULL, 8, &lora_recv_task);
}
static void LLCC68OnTxDone(void)
{
    esp_rom_printf("TxDone\r\n");
    /*
      ???????????????
    */
    // Radio.Standby();

    // // ?????????????led???
    // GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    // delay_ms(100);
    // GPIO_SetBits(GPIOB, GPIO_Pin_12);
}
#define MAX_ARRAY_SIZE 1024 // ????????????
#define MAX_PACKETS 64      // ???????????????MAX_PACKET_SIZE=256????
int16_t received_array[MAX_ARRAY_SIZE];
bool packet_received[MAX_PACKETS] = {0};
size_t received_length = 0;
static void LLCC68OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    // /*
    // ??????????????
    // */
    // // uint32_t reciveNumber = 0;
    // // Radio.Standby();
    // rxRetryCount = 0;
    // esp_rom_printf("size:%d\r\nrssi:%d\r\nsnr:%d\r\npayload:%s\r\n", size, rssi, snr, payload);
    // // memcpy(packet, payload, size);

    // size_t bytes_written = stream_buffer_write((const int16_t *)payload, size);
    // if (bytes_written != size)
    // {
    //     ESP_LOGE("LORA_RECV", "Failed to write all data to stream buffer");
    // }
    // // Radio.Rx(LORA_RX_TIMEOUT_VALUE);
    Packet_t *packet = (Packet_t *)payload;

    // ????????????
    if (size < sizeof(packet->total_packets) + sizeof(packet->packet_index))
    {
        ESP_LOGE("LORA_RECV", "?????????");
        return;
    }

    // ??????????????????
    size_t data_bytes = size - sizeof(packet->total_packets) - sizeof(packet->packet_index);
    size_t elements = data_bytes / sizeof(int16_t);
    // ???????????????????
    const size_t max_elements_per_packet = sizeof(((Packet_t *)0)->data) / sizeof(int16_t);

    // ?????????????????
    if (packet->packet_index >= MAX_PACKETS)
    {
        ESP_LOGE("LORA_RECV", "?????????:%d", packet->packet_index);
        return;
    }

    // ?????????????
    size_t start_idx = packet->packet_index * max_elements_per_packet;
    if ((start_idx + elements) > MAX_ARRAY_SIZE)
    {
        ESP_LOGE("LORA_RECV", "?????????????");
        return;
    }

    // ???????
    memcpy(received_array + start_idx, packet->data, elements * sizeof(int16_t));
    packet_received[packet->packet_index] = true;

    // ???????????
    uint16_t received_packets = 0;
    for (int i = 0; i < MAX_PACKETS; i++)
    {
        if (packet_received[i])
        {
            received_packets++;
        }
    }

    // ?????????????????????
    if (received_packets == packet->total_packets)
    {
        size_t total_elements = packet->total_packets * max_elements_per_packet;
        // process_received_array(received_array, total_elements);

        // ?????????
        memset(packet_received, 0, sizeof(packet_received));
        received_length = 0;
    }

    esp_rom_printf("size:%d\r\nrssi:%d\r\nsnr:%d\r\n", size, rssi, snr);
}

static void LLCC68OnTxTimeout(void)
{

    esp_rom_printf("TxTimeout\r\n");
}

// ??????
static void LLCC68OnRxTimeout(void) // ????????????????
{
    Radio.Standby();
    //   rxRetryCount++;
    //   if (rxRetryCount > MAX_RX_RETRY)
    //   {
    //       LLCC68RxTimerStop();
    //       esp_rom_printf("RX retry exceeded maximum limit. Abort!\n");
    //       rxRetryCount = 0;

    // Radio.Rx(LORA_RX_TIMEOUT_VALUE);
    //       Radio.Standby();
    // Radio.Rx(0);
    //       return; // ????????????
    //   }
    //  else
    //  {

    esp_rom_printf("RxTimeout retry recive\r\n");
    Radio.Rx(LORA_RX_TIMEOUT_VALUE);
    //}
}

static void LLCC68OnRxError(void)
{
    Radio.Standby();
    esp_rom_printf("RxError retry recive\r\n");
    Radio.Rx(LORA_RX_TIMEOUT_VALUE);
}
