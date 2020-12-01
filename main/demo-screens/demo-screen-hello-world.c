#include "demo-screen-hello-world.h"

typedef struct hello_world_data {
    uint32_t call_cnt;
    lv_obj_t *window;
    lv_obj_t *text_area;
} hello_world_data_t;

void hello_world_screen_worker(lv_obj_t *screen, void *priv) {
    hello_world_data_t *pdata = priv;
    char cnt[] = "0xFFFFFFFF";
    sprintf(cnt, "0x%x", pdata->call_cnt++);
    lv_textarea_set_text(pdata->text_area, cnt);
}

void *hello_world_screen_init(lv_obj_t *screen) {
    hello_world_data_t *priv = calloc(1, sizeof(hello_world_data_t));

    priv->window = lv_win_create(screen, NULL);
    lv_win_set_title(priv->window, "Hello World!");

    priv->text_area = lv_textarea_create(priv->window, NULL);
    lv_textarea_set_text(priv->text_area, "Counting starting...");
    return priv;
}
