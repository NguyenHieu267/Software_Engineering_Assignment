#include "coreiot_task.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "app_queues.h"

static const char *TAG = "task_coreiot";

#define WIFI_CONNECTED_BIT BIT0

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_ENDPOINT "http://192.168.1.100:3000/api/events"

static EventGroupHandle_t s_wifi_event_group;

static bool has_wifi_config(void)
{
    return strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");
    return ESP_OK;
}

static void coreiot_http_post(const app_event_t *evt)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return;
    }

    cJSON_AddNumberToObject(root, "event_type", evt->type);
    cJSON_AddStringToObject(root, "uid", evt->uid);
    cJSON_AddNumberToObject(root, "total_spaces", evt->total_spaces);
    cJSON_AddNumberToObject(root, "empty_spaces", evt->empty_spaces);
    cJSON_AddNumberToObject(root, "occupied_spaces", evt->occupied_spaces);
    cJSON_AddBoolToObject(root, "ir_occupied", evt->ir_occupied);
    cJSON_AddNumberToObject(root, "timestamp_ms", (double)evt->timestamp_ms);

    char *payload = cJSON_PrintUnformatted(root);
    if (!payload) {
        cJSON_Delete(root);
        return;
    }

    esp_http_client_config_t config = {
        .url = API_ENDPOINT,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "POST %s -> HTTP %d", API_ENDPOINT, status);
    } else {
        ESP_LOGW(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_free(payload);
    cJSON_Delete(root);
}

void coreIoT(void *pvParameters)
{
    (void)pvParameters;

    bool wifi_enabled = has_wifi_config();
    if (wifi_enabled) {
        ESP_ERROR_CHECK(wifi_init_sta());
    } else {
        ESP_LOGW(TAG, "WiFi credentials are placeholders. CoreIoT will log events only.");
    }

    app_event_t evt;
    while (1) {
        if (xQueueReceive(g_coreiot_evt_queue, &evt, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG,
                     "event=%d uid=%s empty=%d occupied=%d ir=%d ts=%" PRIu64,
                     evt.type,
                     evt.uid,
                     evt.empty_spaces,
                     evt.occupied_spaces,
                     evt.ir_occupied,
                     evt.timestamp_ms);

            if (!wifi_enabled) {
                continue;
            }

            EventBits_t bits = xEventGroupWaitBits(
                s_wifi_event_group,
                WIFI_CONNECTED_BIT,
                pdFALSE,
                pdFALSE,
                pdMS_TO_TICKS(3000)
            );

            if (bits & WIFI_CONNECTED_BIT) {
                coreiot_http_post(&evt);
            } else {
                ESP_LOGW(TAG, "WiFi not connected, skip POST");
            }
        }
    }
}
