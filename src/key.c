#include "key.h"
elab_key_t keys[numofkey] = {
    {0, KEY_NONE, false},
};
void key_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    // 配置Key_Sta为输入
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << Key_Sta);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 配置Key_RD和Key_CLK为输出
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << Key_RD) | (1ULL << Key_CLK));
    gpio_config(&io_conf);

    // 设置初始电平
    gpio_set_level(Key_RD, HIGH);  // 高电平数据不送入移位寄存器
    gpio_set_level(Key_CLK, HIGH); // 上升沿开始移位
}
void key_scan(void *parameter)
{
    uint8_t *localparameter;
    localparameter = (uint8_t *)parameter;
    uint8_t state = *localparameter;
    CLK_H;
    PL_L; // 将数据送入移位寄存器
    vTaskDelay(pdMS_TO_TICKS(2));
    PL_H; // 停止送入数据
    if (gpio_get_level(Key_Sta) == HIGH)
    {
        state |= 0x01;
    }
    for (int i = 0; i < 7; i++)
    {
        state = state << 1;
        CLK_L;
        vTaskDelay(pdMS_TO_TICKS(1));
        CLK_H;
        vTaskDelay(pdMS_TO_TICKS(1));

        if (gpio_get_level(Key_Sta) == HIGH)
        {
            state |= 0x01;
            // keys[i].is_pressed = true;
            // Serial.print("Key ");
            // Serial.print(i);
            // Serial.print(" is pressed: ");
            // Serial.println(keys[i].is_pressed);

            //  keys[i].press_start_time = millis();
        }
    }
    *localparameter = state;
}
