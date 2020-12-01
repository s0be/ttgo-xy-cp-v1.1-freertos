#pragma once

#include "demo-screen-common.h"

void *voltage_screen_init(lv_obj_t *screen);
void voltage_screen_worker(lv_obj_t *screen, void *priv);
void voltage_screen_load(lv_obj_t *screen, void *priv);
void voltage_screen_unload(lv_obj_t *screen, void *priv);