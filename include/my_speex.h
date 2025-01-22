#ifndef _MY_SPEEX_H_
#define _MY_SPEEX_H_

#include <stdio.h>
#include "speex/speex.h"
#include "SPEEX/speex_preprocess.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#define FRAME_SIZE 160
#define SAMPLE_RATE 8000
void Speex_Init(void);

void process_audio_data(int16_t *audio_data);
// ????i??
#endif
