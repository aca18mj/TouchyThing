#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_common_events.h>
#include <app_wifi.h>
#include "driver/gpio.h"
#include "touch.h"
#include "ifttt.h"

#define BUILT_IN_LED 13

static const char *TAG = "app_main";

esp_rmaker_device_t *esp32_device;


/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    
    if (strcmp(esp_rmaker_param_get_name(param), "built_in_LED") == 0)
    {
        // toggle built-in LED when param name is "built_in_LED"
        ESP_LOGI(TAG, "Switch control");
        if (val.val.b)
            gpio_set_level(BUILT_IN_LED, 1);
        else
            gpio_set_level(BUILT_IN_LED, 0);

        esp_rmaker_param_update_and_report(param, val);
    }
    else if (strcmp(esp_rmaker_param_get_name(param), "Tweet") == 0)
    {
        // send a Tweet when param name is "Tweet"
        ESP_LOGI(TAG, "Tweet control");
        send_tweet();

        esp_rmaker_param_update_and_report(param, val);
    }
    else 
    {
        ESP_LOGI(TAG, "Param name not expected: %s", esp_rmaker_param_get_name(param));
    }
        
    return ESP_OK;
}


/* Event handler for catching RainMaker events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_INIT_DONE:
                ESP_LOGI(TAG, "RainMaker Initialised.");
                break;
            case RMAKER_EVENT_CLAIM_STARTED:
                ESP_LOGI(TAG, "RainMaker Claim Started.");
                break;
            case RMAKER_EVENT_CLAIM_SUCCESSFUL:
                ESP_LOGI(TAG, "RainMaker Claim Successful.");
                break;
            case RMAKER_EVENT_CLAIM_FAILED:
                ESP_LOGI(TAG, "RainMaker Claim Failed.");
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Event: %d", event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
            case RMAKER_EVENT_REBOOT:
                ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
                break;
            case RMAKER_EVENT_WIFI_RESET:
                ESP_LOGI(TAG, "Wi-Fi credentials reset.");
                break;
            case RMAKER_EVENT_FACTORY_RESET:
                ESP_LOGI(TAG, "Node reset to factory defaults.");
                break;
            case RMAKER_MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT Connected.");
                break;
            case RMAKER_MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT Disconnected.");
                break;
            case RMAKER_MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
                break;
            default:
                ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %d", event_id);
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}


//Program main entry
void app_main()
{
    // Setup Rainmaker
    esp_rmaker_console_init();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Init WiFi
    app_wifi_init();

    // Register an event handler to catch RainMaker events
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));


    // Initialize the ESP RainMaker Agent
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };

    // Create node to add devices
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP32 RainMaker Device", "LED and Tweeter");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    // Create LED device
    gpio_pad_select_gpio(BUILT_IN_LED);
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);


    esp32_device = esp_rmaker_device_create("ESP32", NULL, NULL);
    esp_rmaker_device_add_cb(esp32_device, write_cb, NULL);

    // Create LED param
    esp_rmaker_param_t *on_param = esp_rmaker_param_create("built_in_LED", NULL, esp_rmaker_bool(false),
                            PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(on_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(esp32_device, on_param);

    
    // Create Tweet param
    esp_rmaker_param_t *tweet_param = esp_rmaker_param_create("Tweet", NULL, esp_rmaker_bool(false),
                            PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(tweet_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(esp32_device, tweet_param);

    esp_rmaker_node_add_device(node, esp32_device);


    // Enable OTA
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    esp_rmaker_schedule_enable();

    esp_rmaker_start();

    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    // Initialise IFTTT module
    ifttt_init();

    // Start reading touch input
    touch_sense_init();
}