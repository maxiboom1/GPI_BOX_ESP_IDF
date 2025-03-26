#include "http_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "app_config.h"  // for globalConfig
#include <string.h>
#include "app_config.h"  // for globalConfig
#include <string.h>

#define TAG "HTTP_CLIENT"
extern AppConfig globalConfig;

// Hardcoded destination URL for now
#define POST_URL "http://10.168.0.10:9000" 

typedef struct {
    char host[64];
    int port;
    char path[64];
} UrlObj;

UrlObj parse_http_url(void) {
    UrlObj result = {
        .host = "",
        .port = 80,       // default port
        .path = "/"
    };

    const char *url = globalConfig.httpUrl;

    // Skip "http://" if present
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }

    const char *slash = strchr(url, '/');
    const char *colon = strchr(url, ':');

    if (slash) {
        strncpy(result.path, slash, sizeof(result.path) - 1);
    }

    size_t host_len = 0;

    if (colon && (!slash || colon < slash)) {
        host_len = colon - url;
        if (slash) {
            result.port = atoi(colon + 1);
        } else {
            result.port = atoi(colon + 1);  // default path is "/"
        }
    } else if (slash) {
        host_len = slash - url;
    } else {
        host_len = strlen(url);
    }

    strncpy(result.host, url, host_len);
    result.host[sizeof(result.host) - 1] = '\0';

    return result;
}

esp_err_t send_http_post(const char *json_data) {
    esp_http_client_config_t config = {
        .url = POST_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 3000
    };

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