

#include "demo-screen-common.h"

#include "task-wifi.h"

typedef struct wifi_screen {
    lv_obj_t *win;
    lv_obj_t *text_area;
    QueueHandle_t msg_queue;
} wifi_screen_t;

void *wifi_screen_init(lv_obj_t *screen) {
    wifi_screen_t *priv = calloc(1, sizeof(wifi_screen_t));

    priv->win = lv_win_create(screen, NULL);
    lv_win_set_title(priv->win, "WiFi!");

    priv->text_area = lv_textarea_create(priv->win, NULL);
    lv_textarea_set_text(priv->text_area, "Wifi starting...");

    priv->msg_queue = wifi_init("SomeSSID", "SomePASS");
    return priv;
}

void wifi_screen_worker(lv_obj_t *screen, void *priv) {
    wifi_screen_t *pdata = priv;
    char *msg = NULL;
    if(xQueueReceive(pdata->msg_queue, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
        lv_textarea_add_text(pdata->text_area, "\n");
        lv_textarea_add_text(pdata->text_area, msg);
        free(msg);
    }
    
}