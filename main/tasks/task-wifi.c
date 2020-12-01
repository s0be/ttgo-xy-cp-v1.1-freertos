#include "task-wifi.h"

#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"

typedef enum event_base {
    UNKNOWN_EVENT = 0,
    OUR_WIFI_EVENT = 1,
    OUR_IP_EVENT = 2
} event_base_t;

event_base_t to_base(esp_event_base_t event_base) {
    event_base_t ret = UNKNOWN_EVENT;
    if (event_base == WIFI_EVENT) ret = OUR_WIFI_EVENT;
    if (event_base == IP_EVENT) ret = OUR_IP_EVENT;
    return ret;
}

static const char *tag = "wifi task";

static void wifi_event_handler(void *arg, int32_t event_id, void *event_data) {
    QueueHandle_t msg_queue = arg;
    char *msg;
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            msg = strdup("try AP connect");
            ESP_LOGI(tag, "%s", msg);
            if(xQueueSend(msg_queue, &msg, pdMS_TO_TICKS(100)) == pdFALSE) {
                free(msg);
            }
            break;
               
        case WIFI_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            msg = strdup("retry AP connect");
            ESP_LOGI(tag, "%s", msg);
            if(xQueueSend(msg_queue, &msg, pdMS_TO_TICKS(100)) == pdFALSE) {
                free(msg);
            }
            break;
    }
}

static void ip_event_handler(void *arg, int32_t event_id, void *event_data) {
    char *msg;
    QueueHandle_t msg_queue = arg;
    switch(event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            msg = strdup("255.255.255.255");
            sprintf(msg, IPSTR, IP2STR(&event->ip_info.ip));
            ESP_LOGI(tag, "got ip: %s", msg);
            if(xQueueSend(msg_queue, &msg, pdMS_TO_TICKS(100)) == pdFALSE) {
                free(msg);
            }
        } break;
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    
    switch(to_base(event_base)) {
        case OUR_WIFI_EVENT:
            wifi_event_handler(arg, event_id, event_data);
            break;
        case OUR_IP_EVENT:
            ip_event_handler(arg, event_id, event_data);
            break;
        case UNKNOWN_EVENT:
            ESP_LOGE(tag, "Unknown base event %s", event_base);
            return;
    }
}

QueueHandle_t wifi_init(char *ssid, char *pass) {
    QueueHandle_t msg_handle = xQueueCreate(10, sizeof(char*));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &event_handler, msg_handle));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &event_handler, msg_handle));

    wifi_config_t wifi_config = {
        .sta = {/* Setting a password implies station will connect to all
                 * security modes including WEP/WPA. However these modes are
                 * deprecated and not advisable to be used. Incase your
                 * Access point doesn't support WPA2, these mode can be
                 * enabled by commenting below line */
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,

                .pmf_cfg = {.capable = true, .required = false}}};

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, pass, strlen(pass));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(tag, "wifi_init_sta finished.");

    return msg_handle;
}