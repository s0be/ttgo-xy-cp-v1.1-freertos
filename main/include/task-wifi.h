#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
QueueHandle_t wifi_init(char *ssid, char *pass);