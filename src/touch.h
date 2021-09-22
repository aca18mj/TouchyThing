#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/touch_pad.h"
#include "driver/gpio.h"

#include "ifttt.h"

#define TOUCHPAD TOUCH_PAD_NUM0
#define FILTER 100

#define GREEN_LED_GPIO 14
#define YELLOW_LED_GPIO 15

#define THRESHOLD 700
#define TRIPLE_TOUCH_SPEED 1000

void vTaskCode(void * pvParameters);

void touch_sense_init();

void setup_touch_pad(touch_pad_t tp_index);

void setup_led();