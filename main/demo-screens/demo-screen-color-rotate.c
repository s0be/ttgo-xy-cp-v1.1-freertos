#include "demo-screen-color-rotate.h"

// Color rotate
typedef struct {
    lv_obj_t *win;
    uint8_t color_index;
    int64_t last_call;
} color_rotate_demo_t;

void *color_rotate_screen_init(lv_obj_t *screen) {
    color_rotate_demo_t *priv = calloc(1, sizeof(color_rotate_demo_t));
    
    priv->win = lv_win_create(screen, NULL);
    lv_win_set_title(priv->win, "Color Cycle!");

    return priv;
}

void color_rotate_screen_worker(lv_obj_t *screen, void *priv) {
    color_rotate_demo_t *pdata = priv;
    
    int64_t now = esp_timer_get_time();
    if ((now - pdata->last_call) < UPDATE_INTERVAL_US) {
        return;
    }

    lv_color_t new_color;
    switch (++pdata->color_index) {
        default:
            pdata->color_index = 0;
            /* fall-through */
        case 0:
            new_color = LV_COLOR_BLACK;
            break;
        case 1:
            new_color = LV_COLOR_BLUE;
            break;
        case 2:
            new_color = LV_COLOR_CYAN;
            break;
        case 3:
            new_color = LV_COLOR_GRAY;
            break;
        case 4:
            new_color = LV_COLOR_GREEN;
            break;
        case 5:
            new_color = LV_COLOR_LIME;
            break;
        case 6:
            new_color = LV_COLOR_MAGENTA;
            break;
        case 7:
            new_color = LV_COLOR_MAROON;
            break;
        case 8:
            new_color = LV_COLOR_NAVY;
            break;
        case 9:
            new_color = LV_COLOR_OLIVE;
            break;
        case 10:
            new_color = LV_COLOR_ORANGE;
            break;
        case 11:
            new_color = LV_COLOR_PURPLE;
            break;
        case 12:
            new_color = LV_COLOR_RED;
            break;
        case 13:
            new_color = LV_COLOR_SILVER;
            break;
        case 14:
            new_color = LV_COLOR_TEAL;
            break;
        case 15:
            new_color = LV_COLOR_WHITE;
            break;
        case 16:
            new_color = LV_COLOR_YELLOW;
            break;
    }

    lv_obj_set_style_local_bg_color(screen, LV_OBJ_PART_MAIN,
                                    LV_STATE_DEFAULT, new_color);
    pdata->last_call = now;
}