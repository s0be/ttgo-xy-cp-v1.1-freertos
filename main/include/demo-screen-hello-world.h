#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl/lvgl.h"

void *hello_world_screen_init(lv_obj_t *screen);
void hello_world_screen_worker(lv_obj_t *screen, void *priv);