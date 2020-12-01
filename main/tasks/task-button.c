#include <limits.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "task-button.h"

static const char *tag = "button_task";

typedef struct isr_event {
    int64_t edge_time;
    int32_t button;
    uint8_t level;
} isr_event_t;

void log_evt(const char *ltag, isr_event_t *evt) {
    ESP_LOGI(ltag, "(%"PRId64") (%i)->%u", evt->edge_time, evt->button, evt->level);
}

typedef struct callback_item {
    struct callback_item *next;
    button_callback_t callback;
    bool pressed;
    uint64_t callbacks;
} callback_item_t;

typedef struct isr_data {
    QueueHandle_t button_queue;
    button_spec_t button_spec;
    uint8_t button;
} isr_data_t;

typedef struct buttons {
    QueueHandle_t button_queue;
    TaskHandle_t button_task;
    isr_data_t **button_data;
    callback_item_t *callback_head;
    int max_buttons;
    int buttons_registered;
} buttons_t;

void IRAM_ATTR button_isr(void *param) {
    isr_data_t *data = (isr_data_t *)param;
    BaseType_t should_wake = pdFALSE;
    isr_event_t evt = {.button = data->button,
                       .level = gpio_get_level(data->button_spec.gpio_num),
                       .edge_time = esp_timer_get_time()};

    xQueueSendFromISR(data->button_queue, &evt, &should_wake);

    if (should_wake == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void setup_interrupts(buttons_handle_t *wdata) {
    (void)wdata;
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
}

int setup_button_gpio(buttons_handle_t button_handle, button_spec_t *button) {
    
    buttons_t *bdata = (buttons_t *)button_handle;
    int button_index = bdata->buttons_registered;

    gpio_set_intr_type(button->gpio_num, GPIO_INTR_ANYEDGE);
    gpio_set_direction(button->gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(button->gpio_num, button->pull_mode);

    isr_data_t *button_isr_data = calloc(1, sizeof(isr_data_t));
    if (button_isr_data == NULL) {
        ESP_LOGE(tag, "ENOMEM allocating button%d data",
                 bdata->buttons_registered);
        vTaskDelay(portMAX_DELAY);
    }

    ESP_LOGI(tag, "Working on bdata: %p", bdata);
    
    button_isr_data->button_queue = bdata->button_queue;
    button_isr_data->button = bdata->buttons_registered;
    memcpy(&button_isr_data->button_spec, button, sizeof(button_spec_t));

    bdata->button_data[bdata->buttons_registered] = button_isr_data;
    bdata->buttons_registered++;

    gpio_isr_handler_add(button->gpio_num, &button_isr, button_isr_data);
    ESP_LOGI(tag, "Attached button %i", button_index);
    return button_index;
}

int64_t get_start_time(int64_t *times, uint64_t button_mask) {
    uint64_t n = button_mask;
    uint8_t start = log2(n & -n);
    int64_t start_time = INT64_MAX;
    for (uint8_t i = start; n & ~((1UL << i) - 1); i++) {
        if (n & (1UL << i)) {
            if (times[i] < start_time) {
                start_time = times[i];
            }
        }
    }
    return start_time;
}

static const char *button_tag = "button_worker";
void button_worker(buttons_handle_t button_handle) {
    buttons_t *bdata = (buttons_t *)button_handle;
    int64_t *start_times = calloc(bdata->max_buttons, sizeof(int64_t));
    uint64_t active_mask = 0;

    while (true) {
        isr_event_t evt = {0};
        if (xQueueReceive(bdata->button_queue, &evt, portMAX_DELAY) == pdTRUE) {
            //log_evt(">", &evt);
            do {
                uint64_t evt_mask = 0;
                
                if(evt.edge_time) {
                    button_spec_t *button_spec =
                        &(bdata->button_data[evt.button]->button_spec);
                    button_active_level_t active_level = button_spec->active_level;
                    bool button_active = (evt.level == active_level);
                    uint64_t button_mask = (1<<evt.button);
                    if(button_active) {
                        if(!(active_mask & button_mask)) {
                            start_times[evt.button] = evt.edge_time;
                        }
                        evt_mask = active_mask | button_mask;
                    } else {
                        evt_mask = active_mask & ~button_mask;
                    }
                } else {
                    evt.edge_time = esp_timer_get_time();
                    evt_mask = active_mask;
                }

                callback_item_t *cb = bdata->callback_head;
                int64_t repeat = 0;
                while(cb != NULL) {
                    int64_t start = get_start_time(start_times, cb->callback.button_mask);
                    int64_t duration = evt.edge_time - start;
                    
                    if (((cb->callback.button_mask & evt_mask) == cb->callback.button_mask) &&
                        ((cb->callback.ignore_mask & evt_mask) == 0)) {
                                                
                        if (!cb->pressed) {
                            if(duration > cb->callback.min_time) {
                                if (cb->callback.press_cb) {
                                    cb->callback.press_cb(start, PRESS, cb->callback.press_param);
                                }
                                cb->pressed = true;
                            } else {
                                repeat = cb->callback.min_time - duration;
                            }
                        }
                        
                        if(cb->callback.held_cb) {
                            if(duration > cb->callback.min_time + cb->callbacks * cb->callback.callback_interval) {
                                cb->callback.held_cb(evt.edge_time, HELD, cb->callback.held_param);
                                cb->callbacks++;
                                repeat = cb->callback.callback_interval;
                            }
                        }
                    } else {
                        if (cb->pressed) {
                            if (cb->callback.release_cb) {
                                if (duration < cb->callback.max_time) {
                                    cb->callback.release_cb(evt.edge_time, RELEASE, cb->callback.release_param);
                                }
                            }
                            cb->pressed = false;
                        }
                        cb->callbacks = 0;
                    }
                    cb = cb->next;
                }

                active_mask = evt_mask;
                if (xQueueReceive(bdata->button_queue, &evt, pdMS_TO_TICKS(repeat/1000)) == pdFALSE) {
                    evt.edge_time = 0;                    
                }
            } while(active_mask);
        }
    }
}

callback_handle_t attach_callback(buttons_handle_t button_handle,
                                  button_callback_t *cb) {
    buttons_t *bdata = (buttons_t *)button_handle;

    callback_item_t *new_cb = calloc(1, sizeof(callback_item_t));
    memcpy(&new_cb->callback, cb, sizeof(button_callback_t));
    
    callback_item_t *insert_at = bdata->callback_head;
    if(insert_at == NULL) {
        bdata->callback_head = new_cb;
    } else {
        while(insert_at->next != NULL) {
            insert_at = insert_at->next;
        }
        insert_at->next = new_cb;
    }
    return new_cb;
}

buttons_handle_t init_buttons(int max_buttons) {
    buttons_t *button_data = calloc(1, sizeof(buttons_t));
    if (button_data == NULL) {
        ESP_LOGE(tag, "Failed to create button_handle");
        vTaskDelay(portMAX_DELAY);
    }

    button_data->max_buttons = max_buttons;
    button_data->button_data = calloc(max_buttons, sizeof(isr_data_t *));
    if(button_data->button_data == NULL) {
        ESP_LOGE(tag, "Failed to create button_data storage");
        vTaskDelay(portMAX_DELAY);
    }

    button_data->button_queue = xQueueCreate(10, sizeof(isr_event_t));
    if (button_data->button_queue == NULL) {
        ESP_LOGE(tag, "Failed to create button_queue");
        vTaskDelay(portMAX_DELAY);
    }

    BaseType_t ret = xTaskCreate(button_worker, button_tag, 2048, button_data,
                                 2, &button_data->button_task);
    if (ret != pdTRUE) {
        ESP_LOGE(tag, "Failed to create the button_task");
        vTaskDelay(portMAX_DELAY);
    }

    ESP_LOGI(tag, "Allocated button_data: %p", button_data);
    return button_data;
}