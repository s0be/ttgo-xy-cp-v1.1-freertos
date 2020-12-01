#include "demo-screen-voltage.h"

#include "task-voltage.h"

typedef struct voltage_screen {
    lv_obj_t *win;
    lv_obj_t *text_area;
    adc_handle_t adc_data;
    QueueHandle_t readings;
} voltage_screen_t;

void *voltage_screen_init(lv_obj_t *screen) {
    voltage_screen_t *priv = calloc(1, sizeof(voltage_screen_t));

    priv->win = lv_win_create(screen, NULL);
    lv_win_set_title(priv->win, "Voltage!");

    priv->text_area = lv_textarea_create(priv->win, NULL);
    lv_textarea_set_text(priv->text_area, "Voltage starting...");

    priv->readings = voltage_worker_init(&priv->adc_data);
    return priv;
}

void voltage_screen_worker(lv_obj_t *screen, void *priv) {
    voltage_screen_t *pdata = priv;
    adc_reading_t newval = {0};
    if(xQueueReceive(pdata->readings, &newval, 0) == pdTRUE) {
        char volts[] = "-0.000V";
        sprintf(volts, "%0.3fV", newval.reading);
        lv_textarea_set_text(pdata->text_area, volts);
        if(newval.charging) {
            lv_textarea_add_text(pdata->text_area, "\nCharging");
        } else {
            lv_textarea_add_text(pdata->text_area, "\nDischarging");
        }
    }
}

void voltage_screen_load(lv_obj_t *screen, void *priv) {
    voltage_screen_t *pdata = priv;
    voltage_task_start(pdata->adc_data);
}

void voltage_screen_unload(lv_obj_t *screen, void *priv) {
    voltage_screen_t *pdata = priv;
    voltage_task_stop(pdata->adc_data);
}