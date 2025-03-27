#include "http_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "app_config.h"  // for globalConfig
#include <string.h>

#define TAG "HTTP_CLIENT"
extern AppConfig globalConfig;

esp_err_t send_http_post(const char *json_data) {
    esp_http_client_config_t config = { .url = globalConfig.httpUrl, .method = HTTP_METHOD_POST, .timeout_ms = 3000 };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "POST sent successfully. Status = %d",
                 esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}