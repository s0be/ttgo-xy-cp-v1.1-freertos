#pragma once

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

typedef struct {
    float reading;
    bool charging;
} adc_reading_t;

typedef void *adc_handle_t;

QueueHandle_t voltage_worker_init(adc_handle_t *data);
void voltage_task_worker(void *param);
void voltage_task_start(adc_handle_t data);
void voltage_task_stop(adc_handle_t data);