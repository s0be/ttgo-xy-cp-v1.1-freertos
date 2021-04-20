#include "task-wifi.h"

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mdf_common.h"
#include "mwifi.h"
#include "nvs_flash.h"

typedef struct wifi_private {
    esp_netif_t *netif_sta;
    esp_netif_t *netif_ap;
    QueueHandle_t msg_queue;
} wifi_private_t;

wifi_private_t *priv = NULL;

static const char *tag = "wifi task";
#define TAG tag

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
    char *msg;
    wifi_private_t *wpriv = arg;
    switch(event_id) {
        case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                static const char *fmt = "STA ip: " IPSTR;
                size_t len = snprintf(NULL, 0, fmt, IP2STR(&event->ip_info.ip));
                msg = calloc(len + 1, sizeof(char));
                sprintf(msg, fmt, IP2STR(&event->ip_info.ip));

                ESP_LOGI(tag, "%s", msg);
                if (xQueueSend(wpriv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                    pdFALSE) {
                    free(msg);
                }
                // esp_err_t esp_netif_get_ip_info(esp_netif_t * esp_netif,
                // esp_netif_ip_info_t * ip_info);
                
                static const char *apfmt = "AP ip: " IPSTR;
                esp_netif_ip_info_t ip_info = {0};
                esp_netif_get_ip_info(wpriv->netif_ap, &ip_info);

                len = snprintf(NULL, 0, apfmt, IP2STR(&ip_info.ip));
                msg = calloc(len + 1, sizeof(char));
                sprintf(msg, apfmt, IP2STR(&ip_info.ip));

                ESP_LOGI(tag, "%s", msg);
                if (xQueueSend(wpriv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                    pdFALSE) {
                    free(msg);
                }
                
        } break;
        case IP_EVENT_AP_STAIPASSIGNED: {
            ip_event_ap_staipassigned_t *event = event_data;
            static const char *fmt = "Client ip: " IPSTR;
            size_t len = snprintf(NULL, 0, fmt, IP2STR(&event->ip));
            msg = calloc(len + 1, sizeof(char));
            sprintf(msg, fmt, IP2STR(&event->ip));
            if (xQueueSend(wpriv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                pdFALSE) {
                free(msg);
            }
        }

    }
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx) {
    char *msg = NULL;
    switch (event) {
        case MDF_EVENT_MWIFI_ROOT_ADDRESS:
            if (esp_mesh_is_root()) {
                esp_netif_dhcps_start(priv->netif_ap);
                msg = strdup("Root starting dhcp on AP");
                MDF_LOGI("%s", msg);
                if (xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                    pdFALSE) {
                    free(msg);
                }
            }
            break;
        case MDF_EVENT_MWIFI_STARTED:
            msg = strdup("MESH is started");
            MDF_LOGI("%s", msg);
            if(xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) == pdFALSE) {
                free(msg);
            }
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            msg = strdup("Parent connected");
            MDF_LOGI("%s", msg);

            if (xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                pdFALSE) {
                free(msg);
            }

            
                esp_netif_dhcpc_start(priv->netif_sta);
            

            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            msg = strdup("Parent disconnected");
            MDF_LOGI("%s", msg);

            if (xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                pdFALSE) {
                free(msg);
            }
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE: {
            size_t len = snprintf(NULL, 0, "total_num: %d", esp_mesh_get_total_node_num());
            msg = calloc(len + 1, sizeof(char));
            sprintf(msg, "total_num: %d", esp_mesh_get_total_node_num());
            MDF_LOGI("%s", msg);

            if (xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                pdFALSE) {
                free(msg);
            }
        }
            break;
        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            if(!esp_mesh_is_root()) {
                msg = strdup("Root got IP");
                MDF_LOGI("%s", msg);
                if (xQueueSend(priv->msg_queue, &msg, pdMS_TO_TICKS(10)) ==
                    pdFALSE) {
                    free(msg);
                }
            }
            break;
        }
        
        default:
            break;
    }

    return MDF_OK;
}

QueueHandle_t wifi_init(char *ssid, char *pass) {
    QueueHandle_t msg_handle = xQueueCreate(10, sizeof(char*));
    priv = calloc(1, sizeof(wifi_private_t));
    priv->msg_queue = msg_handle;

    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config = {0};

    memcpy(config.router_ssid, ssid, strlen(ssid));
    memcpy(config.router_password, pass, strlen(pass));

    // Sets the meshid to the first 6 chars of the ssid
    memcpy(config.mesh_id, "******", 6);
    memcpy(config.mesh_id, ssid, strlen(ssid) < 6 ? strlen(ssid) : 6);
    
    // Sets the meshpass to m + the wifi password
    memcpy(config.mesh_password, "m", 1);
    memcpy(config.mesh_password + 1, pass, strlen(pass));

    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    ESP_ERROR_CHECK(
        esp_netif_create_default_wifi_mesh_netifs(&priv->netif_sta, &priv->netif_ap));
    MDF_ERROR_ASSERT(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               ip_event_handler, priv));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED,
                                               ip_event_handler, priv));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());

    return msg_handle;
}