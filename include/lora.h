#ifndef _LORA_H_
#define _LORA_H_
#include "radio.h"
#include "my_llcc68.h"
#include "llcc68_board.h"
#include <stdio.h>
#include <string.h>
extern TaskHandle_t lora_send_task;
extern TaskHandle_t lora_recv_task;
void ExampleLLCC68Sendtask(void);
void ExampleLLCC68Recivetask(void);
void lora_send(void *parameter);
void lora_recv(void *parameter);
#define MAX_PACKET_SIZE 250 // 根据LoRa的最大有效载荷大小调整

#pragma pack(push, 1)
typedef struct
{
    uint16_t total_packets;                  // ?????
    uint16_t packet_index;                   // ????????
    int16_t data[(MAX_PACKET_SIZE - 4) / 2]; // ????????MAX_PACKET_SIZE??????????
} Packet_t;
#pragma pack(pop)
#endif