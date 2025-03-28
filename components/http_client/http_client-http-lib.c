#include "http_client.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "app_config.h"
#include <string.h>

#define TAG "HTTP_CLIENT"

extern AppConfig globalConfig;

typedef struct {
    char json_data[256];
} HttpPostTaskArgs; 

static void http_post_task(void *arg) {
    HttpPostTaskArgs *args = (HttpPostTaskArgs *)arg;

    esp_http_client_config_t config = {
        .url = globalConfig.httpUrl,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 0,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, args->json_data, strlen(args->json_data));
        esp_http_client_perform(client);
        esp_http_client_cleanup(client);
    }

    free(arg);  // Clean up allocated memory
    vTaskDelete(NULL);  // Kill the task
}

esp_err_t send_http_post(const char *json_data) {
    HttpPostTaskArgs *args = malloc(sizeof(HttpPostTaskArgs));
    if (!args) return ESP_ERR_NO_MEM;

    strncpy(args->json_data, json_data, sizeof(args->json_data) - 1);
    args->json_data[sizeof(args->json_data) - 1] = '\0';

    // Create detached task to handle POST
    if (xTaskCreate(http_post_task, "http_post_task", 4096, args, 5, NULL) != pdPASS) {
        free(args);
        return ESP_FAIL;
    }

    return ESP_OK;
}