#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "task-voltage.h"

#define BAT_ADC ADC1_CHANNEL_6
#define BAT_ADC_EN GPIO_NUM_14
#define ADC_READ_INTERVAL_MS 1000

typedef enum adc_worker_message {
    ADC_STOP = 0,
    ADC_START = 1
} adc_worker_message_t;

typedef struct adc_data {
    TaskHandle_t voltage_task;
    QueueHandle_t voltage_event_queue;
    QueueHandle_t readings_queue;
    esp_adc_cal_characteristics_t caldata;
} adc_data_t;

void disable_adc() {
    gpio_set_level(BAT_ADC_EN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
}

void enable_adc() {
    gpio_set_level(BAT_ADC_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
}

static const char *voltage_tag = "voltage_worker";
QueueHandle_t voltage_worker_init(adc_handle_t *data) {
    adc_data_t *adc_data = calloc(1, sizeof(adc_data_t));
    if(adc_data == NULL) {
        ESP_LOGE(voltage_tag, "ENOMEM Allocating ADC Data");
        vTaskDelay(portMAX_DELAY);
    }

    adc_data->voltage_event_queue =
        xQueueCreate(1, sizeof(adc_worker_message_t));
    if (adc_data->voltage_event_queue == NULL) {
        ESP_LOGE(voltage_tag,  "Failed to create voltage queue");
        vTaskDelay(portMAX_DELAY);
    }

    adc_data->readings_queue = 
        xQueueCreate(1, sizeof(adc_reading_t));

    gpio_set_direction(BAT_ADC_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(BAT_ADC_EN, 0);

    // VDDA is wired to 3v3
    // ADC1_CHANNEL_6 is wired to a voltage divider between 'BAT' and gnd
    // BAT will either be:
    //     <4.3v when running on battery and BAT_ADC_EN is high,
    //     <5.1v when running off of usb or external power.
    // Setting 11 DB Attenuation gives us:
    //     0v - 2.6v
    // Which is 0v-5.2v with the voltage divider on the pin.
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BAT_ADC, ADC_ATTEN_DB_11);

    esp_adc_cal_value_t val_type =
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12,
                                 1100, &adc_data->caldata);
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(voltage_tag,  "Using adc calibration from eFuses");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(voltage_tag,  "Using Two Point adc calibration");
    } else {
        ESP_LOGI(voltage_tag, "Using Default ADC Calibration");
    }

    BaseType_t ret = xTaskCreate(&voltage_task_worker, voltage_tag, 2048, adc_data, 2,
                                 &adc_data->voltage_task);
    if (ret != pdTRUE) {
        ESP_LOGE(voltage_tag, "Failed to create the voltage_task");
        vTaskDelay(portMAX_DELAY);
    }
    ESP_LOGI(voltage_tag, "Done creating voltage task");

    *data = adc_data;
    return adc_data->readings_queue;
}

void voltage_task_worker(void *param) {
    adc_data_t *adc_data = (adc_data_t *)param;
    adc_worker_message_t msg = ADC_STOP;

    TickType_t wait_interval = portMAX_DELAY;
    while (1) {
        if (xQueueReceive(adc_data->voltage_event_queue, &msg,
                          wait_interval) == pdFALSE) {
            if (msg == ADC_STOP) {
                ESP_LOGI(voltage_tag, "Timed out waiting for update");
                continue;
            }
        }

        switch (msg) {
            case ADC_STOP:
                wait_interval = portMAX_DELAY;
                break;
            case ADC_START:
                wait_interval = pdMS_TO_TICKS(ADC_READ_INTERVAL_MS);
                
                int raw = 0;
                uint32_t calibrated = 0;
                adc_reading_t reading = {0};
                
                // Check to see if we have external power.
                enable_adc();
                raw = adc1_get_raw(BAT_ADC);

                // Get calibrated value in mV
                calibrated = esp_adc_cal_raw_to_voltage(raw, &adc_data->caldata);
                calibrated *= 2;

                reading.reading = (float)calibrated / 1000;
                reading.charging = reading.reading > 4.5f;

                xQueueOverwrite(adc_data->readings_queue, &reading);

                disable_adc();
                break;
        }
    }
}

void voltage_task_start(adc_handle_t _data) {
    adc_data_t *data =_data;
    adc_worker_message_t start = ADC_START;
    xQueueSend(data->voltage_event_queue, &start, portMAX_DELAY);
}

void voltage_task_stop(adc_handle_t _data) {
    adc_data_t *data = _data;
    adc_worker_message_t stop = ADC_STOP;
    xQueueSend(data->voltage_event_queue, &stop, portMAX_DELAY);
}