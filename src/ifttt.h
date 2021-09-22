#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include <esp_http_client.h>
#include <string.h>
#include "private.h"

#define RED_LED_GPIO 33
#define SMALL_GREEN_LED_GPIO 27


void ifttt_init();

void send_tweet();
