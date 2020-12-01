#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lvgl/lvgl.h"

typedef enum display_mode {
    COLOR_ROTATE,
    HELLO_WORLD,
    VOLTAGE,
    WIFI,
    MAX_DISPLAY_MODE = WIFI
} display_mode_t;

typedef void *screen_handle_t;
typedef void *display_handle_t;

typedef void (*tick_callback_t)(lv_obj_t *screen, void *priv);

display_handle_t init_display(int screen_count);
void show_display(display_handle_t disp_handle, display_mode_t disp);