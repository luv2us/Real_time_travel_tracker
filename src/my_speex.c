
#include "my_speex.h"
SpeexBits bitsEncode;
SpeexBits bitsDecode;

SpeexPreprocessState *preprocess_state = NULL;

void Speex_Init(void)
{
    // speex_bits_init(&bitsEncode);
    // speex_bits_init(&bitsDecode);
    preprocess_state = speex_preprocess_state_init(FRAME_SIZE, SAMPLE_RATE);
    if (preprocess_state == NULL)
    {
        ESP_LOGE("SPEEX", "Failed to initialize preprocess state");
        return;
    }

    // ?
    int denoise = 1;
    int noise_suppress = -30;
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noise_suppress);

    ESP_LOGI("SPEEX", "Speex initialized successfully");
}

void process_audio_data(int16_t *audio_data)
{
    if (preprocess_state == NULL || audio_data == NULL)
    {
        ESP_LOGE("SPEEX", "Invalid state or audio data");
        return;
    }

    int ret = speex_preprocess_run(preprocess_state, audio_data);
    if (ret != 1)
    {
        ESP_LOGE("SPEEX", "Preprocess failed: %d", ret);
    }
}
