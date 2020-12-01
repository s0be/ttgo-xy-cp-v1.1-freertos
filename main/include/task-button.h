#pragma once

#include "driver/gpio.h"
#include "freertos/queue.h"
#include "stdint.h"

typedef enum button_active_level {
    LOW,
    HIGH
} button_active_level_t;

typedef enum event { PRESS, HELD, RELEASE } event_t;

typedef void *buttons_handle_t;
typedef void *button_callback_param_t;
typedef void (*button_callback_func_t)(int64_t event_time, event_t evt,
                                       button_callback_param_t param);
typedef void *callback_handle_t;

typedef struct button_spec {
    gpio_num_t gpio_num;
    gpio_pull_mode_t pull_mode;
    button_active_level_t active_level;
} button_spec_t;

typedef struct button_callback {
    // All times in US
    int64_t min_time; // Minimum press duration to be considered for 'press'
    int64_t max_time; // Maximum press duration to be considered for 'release'
    int64_t callback_interval; // How often after 'press' until 'release' to receive callbacks

    uint64_t button_mask;
    uint64_t ignore_mask;

    button_callback_func_t press_cb;
    button_callback_param_t press_param;
    button_callback_func_t held_cb;
    button_callback_param_t held_param;
    button_callback_func_t release_cb;
    button_callback_param_t release_param;
} button_callback_t;

buttons_handle_t init_buttons(int max_buttons);
void setup_interrupts(buttons_handle_t *wdata);
int setup_button_gpio(buttons_handle_t data, button_spec_t *button);
callback_handle_t attach_callback(buttons_handle_t data, button_callback_t *cb);