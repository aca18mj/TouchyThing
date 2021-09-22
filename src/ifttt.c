#include "ifttt.h"

static const char *TAG = "IFTTT";

/* Sets up the pinouts */
void ifttt_setup_led()
{
    gpio_pad_select_gpio(SMALL_GREEN_LED_GPIO);
    gpio_set_direction(SMALL_GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(RED_LED_GPIO);
    gpio_set_direction(RED_LED_GPIO, GPIO_MODE_OUTPUT);
}


/* Blink given LED twice */
void blink_LED(gpio_num_t led)
{
    gpio_set_level(led, 1);
    vTaskDelay(100);
    gpio_set_level(led, 0);
    vTaskDelay(100);
    gpio_set_level(led, 1);
    vTaskDelay(100);
    gpio_set_level(led, 0);
}

/* Initialise the IFTTT module */
void ifttt_init()
{
    ESP_LOGI(TAG, "IFTTT init");
    ifttt_setup_led();
}

/* Send the tweet through IFTTT service */
void send_tweet()
{
    char url[] = "https://maker.ifttt.com/trigger/esp32_tweet/with/key/";
    strcat(url, IFTTT_KEY);

    // Create HTTP client
    esp_http_client_config_t config = {
            .url = url,
        };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {   
        // If all okay, blink green LED
        blink_LED(SMALL_GREEN_LED_GPIO);
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    } else
    {
        // If not okay, blink red LED
        blink_LED(RED_LED_GPIO);
    }

    esp_http_client_cleanup(client);
}