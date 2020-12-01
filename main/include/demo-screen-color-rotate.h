#pragma once

#include <stdint.h>

#include "demo-screen-common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl/lvgl.h"

// Color changing interval in microseconds
#define UPDATE_INTERVAL_US 1000LL * 1000LL

void *color_rotate_screen_init(lv_obj_t *screen);
void color_rotate_screen_worker(lv_obj_t *screen, void *priv);