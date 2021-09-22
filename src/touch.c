#include "touch.h"

static const char *TAG = "touch";

// Initialise touch pad
void setup_touch_pad(touch_pad_t tp_index)
{
    ESP_ERROR_CHECK(touch_pad_init());
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    touch_pad_config(tp_index, 0);
    touch_pad_filter_start(FILTER);
}

// Setup led pinouts
void setup_led()
{
    gpio_pad_select_gpio(GREEN_LED_GPIO);
    gpio_set_direction(GREEN_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(YELLOW_LED_GPIO);
    gpio_set_direction(YELLOW_LED_GPIO, GPIO_MODE_OUTPUT);
}

// Blinks given LED three times
void three_blinks(gpio_num_t led)
{
    for (int i = 0; i < 3; i++)
    {
        gpio_set_level(led, 1);
        vTaskDelay(20);
        gpio_set_level(led, 0);
        vTaskDelay(20);
    }
}

/*  Sets up the touch pad and LED pinouts, then handles touch input
    (Three consecutive touches to send a tweet) */
void vTaskCode(void * pvParameters)
{
    setup_touch_pad(TOUCHPAD);
    setup_led();

    uint16_t touch_value;

    bool isTouched = false;
    uint8_t n_touches = 0;
    uint32_t last_touch_time = esp_log_timestamp();

    for ( ;; )
    {
        // Reset number of touches when too much delay between touches
        if (!isTouched && n_touches > 0 && esp_log_timestamp() - last_touch_time > TRIPLE_TOUCH_SPEED)
            n_touches = 0;

        // Read touch value
        touch_pad_read_filtered(TOUCHPAD, &touch_value);
        

        if (touch_value < THRESHOLD && !isTouched)
        {
            // Touch registered
            isTouched = true;
            gpio_set_level(GREEN_LED_GPIO, 1);
        }
        else if (touch_value > THRESHOLD && isTouched)
        {   
            // User stopped touching
            n_touches++;

            // Save the time of the last touch
            last_touch_time = esp_log_timestamp();

            gpio_set_level(GREEN_LED_GPIO, 0);


            // If third touch -> send tweet via IFTTT module (and blink yellow node)
            if (n_touches == 3)
            {
                three_blinks(YELLOW_LED_GPIO);
                n_touches = 0;
                send_tweet();
            }

            isTouched = false;
        }

        vTaskDelay(10);
    }
    
}


// Initialises the touch module
void touch_sense_init()
{
    ESP_LOGI(TAG, "Touchy thing init");


    // Create new task to handle touch input
    TaskHandle_t xHandle = NULL;

    xTaskCreate(vTaskCode, "Touch", 4096, (void *) 1, 20, &xHandle);
    configASSERT(xHandle);
}