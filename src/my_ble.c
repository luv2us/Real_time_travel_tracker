
#include "my_ble.h"
#include "nvs_flash.h"
#include "ring_buffer.h"
#define TAG "BLE_APP"
char packet[100] = "hello world";
#define GATTS_TAG "GATTS_DEMO:"

// 1. primary_service_uuid - UUID (0x2800)
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE; // 0x2800

// 2. MASTER_SERVICE_UUID - 128-bit
static const uint8_t master_service_uuid[ESP_UUID_LEN_128] = {
    0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12, 0x03, 0x02, 0x25, 0x20};

// 3. character_declaration_uuid - UUID (0x2803)
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE; // 0x2803

// 4. char_property -
static const uint8_t char_property = ESP_GATT_CHAR_PROP_BIT_READ |
                                     ESP_GATT_CHAR_PROP_BIT_WRITE |
                                     ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// 5. CHARACTERISTIC_UUID - 128-bit
static const uint8_t characteristic_uuid[ESP_UUID_LEN_128] = {
    0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01,
    0x89, 0x67, 0x45, 0x23, 0x01, 0xef, 0xcd, 0xab};

// 6. GATTS_Demo_CHAR_VAL_LEN_MAX
#define GATTS_Demo_CHAR_VAL_LEN_MAX 500

// static uint16_t gatt_service_handle;
// static uint16_t gatt_char_handle;

TaskHandle_t xBleSendGpsDataTaskHandle = NULL;
esp_gatt_if_t my_gatts_if = ESP_GATT_IF_NONE;
static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void ble_gap_config_adv_data()
{
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = 0,       // Manufacturer data length
        .p_manufacturer_data = NULL, // Manufacturer data pointer
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 16,
        .p_service_uuid = (uint8_t *)master_service_uuid,
        .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
    };

    esp_ble_gap_config_adv_data(&adv_data);
}

static uint16_t handle_table[3];
static void ble_create_service(esp_gatt_if_t gatts_if)
{
    esp_gatts_attr_db_t gatt_db[] = {
        // Service Declaration
        [0] = {
            .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
            .att_desc = {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&primary_service_uuid,
                .perm = ESP_GATT_PERM_READ,
                .max_length = sizeof(uint16_t),
                .length = sizeof(master_service_uuid),
                .value = (uint8_t *)&master_service_uuid, // ???UUID
            },
        },
        // Characteristic Declaration
        [1] = {
            .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
            .att_desc = {
                .uuid_length = ESP_UUID_LEN_16,
                .uuid_p = (uint8_t *)&character_declaration_uuid, // ???0x2803
                .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,

                .max_length = sizeof(uint8_t),      // ?????
                .length = sizeof(char_property),    // 1??? + 2??UUID??
                .value = (uint8_t *)&char_property, // ??? + ???????????...
            },
        },
        // Characteristic Value
        [2] = {
            .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
            .att_desc = {
                .uuid_length = ESP_UUID_LEN_128, .uuid_p = (uint8_t *)&characteristic_uuid, .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, .max_length = GATTS_Demo_CHAR_VAL_LEN_MAX,
                .length = strlen(packet),   // ??????
                .value = (uint8_t *)packet, // ???
            },
        },
    };
    esp_err_t ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, 3, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE("BLE", "Failed to create attribute table: %s", esp_err_to_name(ret));
    }
}
void ble_send_gps_data(void *pvParameters)
{
    uint16_t conn_id = (uint16_t)(uintptr_t)(pvParameters);
    static int16_t buffer[512]; // ???????????????
    while (1)
    {
        if (xBleSendGpsDataTaskHandle != NULL)
        {
            // esp_ble_gatts_set_attr_value(handle_table[2], strlen(packet), (uint8_t *)packet);
            // esp_ble_gatts_send_indicate(my_gatts_if, conn_id, handle_table[2], strlen(packet), (uint8_t *)packet, false);
            // ????????
            size_t bytes_read = stream_buffer_read(buffer, sizeof(buffer) / sizeof(int16_t));
            if (bytes_read > 0)
            {
                // ????????????
                esp_ble_gatts_set_attr_value(handle_table[2], bytes_read * sizeof(int16_t), (uint8_t *)buffer);
                esp_err_t ret = esp_ble_gatts_send_indicate(my_gatts_if, conn_id, handle_table[2], bytes_read * sizeof(int16_t), (uint8_t *)buffer, false);
                esp_rom_printf("ret:%d\n", ret);
            }
            else
            {
                // esp_rom_printf("No data to send\n");
                //  ???????????
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
        else
        {

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "Advertising started");
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "Advertising stopped");
        break;
    default:
        break;
    }
}
void gatt_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "GATT server registered, gatts_if = %d", gatts_if);

        ble_create_service(gatts_if);
        ble_gap_config_adv_data();
        esp_ble_gap_start_advertising(&adv_params);

        break;

    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        if (param->add_attr_tab.status == ESP_GATT_OK)
        {
            esp_rom_printf("GATT table attribute tab created\n");
            memcpy(handle_table, param->add_attr_tab.handles, sizeof(handle_table));
            esp_ble_gatts_start_service(handle_table[0]);
        }
        break;

    case ESP_GATTS_CONNECT_EVT:
        esp_rom_printf("GATTS", "Device connected\n");
        if (xBleSendGpsDataTaskHandle == NULL)
        {
            // printf("connid%d\n", (param->connect.conn_id));
            // printf("gatts_if %d\n", gatts_if);
            my_gatts_if = gatts_if;
            xTaskCreate(ble_send_gps_data, "ble_send_gps_data", 4096, (void *)(uintptr_t)(param->connect.conn_id), 5, &xBleSendGpsDataTaskHandle);
        }
        else
        {
            vTaskResume(xBleSendGpsDataTaskHandle);
        }
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI("GATTS", "Device disconnected");
        if (xBleSendGpsDataTaskHandle)
        {
            vTaskSuspend(xBleSendGpsDataTaskHandle);
        }
        esp_ble_gap_start_advertising(&adv_params); // Restart advertising
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.need_rsp)
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }
        ESP_LOGI("GATTS", "Write event, value:");
        for (int i = 0; i < param->write.len; i++)
        {
            printf("%c", param->write.value[i]);
        }
        printf("\n");
        break;

    default:
        break;
    }
}

void ble_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ret = esp_ble_gatts_register_callback(gatt_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(0x55);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "GATT app register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatt_set_local_mtu(40);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "set local MTU failed, error code = %x", ret);
    }
}
