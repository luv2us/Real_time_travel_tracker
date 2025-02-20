#include "driver/uart.h"
#include "driver/gpio.h"
#include "my_gps.h"
#include "my_ble.h"
// ???????

device_data_t gps_data;
SemaphoreHandle_t gps2lora_semaphore = NULL;
char nmea[NMEA_MAX_LENGTH];
char last_parsed_nmea[NMEA_MAX_LENGTH];
uint16_t bufferIndex = 0;
void GPS_Task(void *pvParameters)
{
    uint8_t data[128];
    int length = 0;

    while (1)
    {
        // if (xSemaphoreTake(gps2lora_semaphore, portMAX_DELAY) == pdTRUE)
        //{

        length = uart_read_bytes(GPS_UART_NUM, data, sizeof(data), 20 / portTICK_PERIOD_MS);

        if (length > 0)
        {
            for (int i = 0; i < length; i++)
            {
                char c = data[i];

                if (c == '\n')
                {
                    nmea[bufferIndex] = '\0';

                    if (bufferIndex > 6)
                    {
                        parseGPS(nmea);
                    }
                    bufferIndex = 0;
                }
                else
                {

                    if (bufferIndex < (NMEA_MAX_LENGTH - 1))
                    {
                        nmea[bufferIndex++] = c;
                    }
                    else
                    {
                        bufferIndex = 0;
                    }
                }
            }
        }
        if (gps_changed == true)
        {
            xSemaphoreGive(gps2lora_semaphore);
        }
        // }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void gps_uart_init(void)
{
    // ????????
    // ??????????????
    gps2lora_semaphore = xSemaphoreCreateBinary();
    if (gps2lora_semaphore == NULL)
    {
        esp_rom_printf("Create gps2lora_semaphore failed\n");
    }
    // UART?????
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // ??UART??
    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));

    // ??UART??
    ESP_ERROR_CHECK(uart_set_pin(GPS_UART_NUM, GPS_TXD_PIN, GPS_RXD_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // ??UART??
    ESP_ERROR_CHECK(uart_driver_install(GPS_UART_NUM, GPS_UART_BUFFER_SIZE,
                                        GPS_UART_BUFFER_SIZE, 0, NULL, 0));

    // ??GPS??
    xTaskCreate(GPS_Task, "gps_task", 4096, NULL, tskIDLE_PRIORITY + 5, NULL);

    // ???????
    strncpy(gps_data.devname, "esp32_gps", sizeof(gps_data.devname) - 1);
    gps_data.devname[sizeof(gps_data.devname) - 1] = '\0';
    esp_rom_printf("GPS init success\n");
}

float convertToDecimal(char *coordinate, char direction)
{
    float raw_value;
    sscanf(coordinate, "%f", &raw_value);

    int degrees = (int)(raw_value / 100);        // ??
    float minutes = raw_value - (degrees * 100); // ??

    float decimal = degrees + (minutes / 60.0);

    if (direction == 'S' || direction == 'W')
    {
        decimal = -decimal;
    }

    return decimal;
}
// ??????????ESP-IDF?
static void float_to_str(float value, char *buffer, int precision)
{
    snprintf(buffer, 16, "%.*f", precision, value);
}

uint8_t parseGNGGA(char *msg)
{
    char latitude[20], longitude[20];
    char lat_dir, lon_dir;

    if (sscanf(msg, "$GNGGA,%*f,%[^,],%c,%[^,],%c",
               latitude, &lat_dir, longitude, &lon_dir) == 4)
    {
        float lat = convertToDecimal(latitude, lat_dir);
        float lon = convertToDecimal(longitude, lon_dir);

        // ????????
        float_to_str(lat, gps_data.latitudeStr, 6);
        float_to_str(lon, gps_data.longitudeStr, 6);

        // ??????
        snprintf(packet, sizeof(packet), "ID01:%s%c,%s%c",
                 gps_data.latitudeStr, lat_dir,
                 gps_data.longitudeStr, lon_dir);
        //  esp_rom_printf("GPS data: Latitude: %s, Longitude: %s\n", gps_data.latitudeStr, gps_data.longitudeStr);
        return 0;
    }
    else
    {

        printf("Failed to parse GNGGA sentence.\n");
        return 1;
    }
}
uint8_t parseGNGLL(char *msg)
{
    char latitude[20], longitude[20];
    char lat_direction, lon_direction;

    // ????
    if (sscanf(msg, "$GNGLL,%[^,],%c,%[^,],%c", latitude, &lat_direction, longitude, &lon_direction) == 4)
    {
        // ????
        // printf("Raw Latitude: %s %c, Raw Longitude: %s %c\n", latitude, lat_direction, longitude, lon_direction);

        // ??????
        float lat = convertToDecimal(latitude, lat_direction);
        float lon = convertToDecimal(longitude, lon_direction);

        // ????????
        float_to_str(lat, gps_data.latitudeStr, 6);
        float_to_str(lon, gps_data.longitudeStr, 6);

        // ??????
        snprintf(packet, sizeof(packet), "ID01:%s%c,%s%c",
                 gps_data.latitudeStr, lat_direction,
                 gps_data.longitudeStr, lon_direction);
        //  esp_rom_printf("GPS data: Latitude: %s, Longitude: %s\n", gps_data.latitudeStr, gps_data.longitudeStr);
        return 0;
    }
    else
    {

        printf("Failed to parse GNGLL sentence.\n");
        return 1;
    }
}
bool gps_changed = true;
void parseGPS(char *msg)
{
    uint8_t ret = 1;
    if (msg[0] == '$' && msg[1] == 'G' && msg[2] == 'N' && msg[3] == 'G' && msg[4] == 'G' && msg[5] == 'A')
    {

        ret = parseGNGGA(msg);
    }
    else
    {
        if (msg[0] == '$' && msg[1] == 'G' && msg[2] == 'N' && msg[3] == 'G' && msg[4] == 'L' && msg[5] == 'L')
        {
            ret = parseGNGLL(msg);
        }
    }
    if (strcmp(last_parsed_nmea, packet) == 0 && ret == 0)
    {
        gps_changed = false;
        return;
    }
    else
    {
        if (ret == 0)
        {
            // printf("GPS:%s\n", msg);
            // printf("last_parsed_nmea:%s\n", last_parsed_nmea);
            gps_changed = true;
            memcpy(last_parsed_nmea, packet, sizeof(packet));
        }
    }
}
