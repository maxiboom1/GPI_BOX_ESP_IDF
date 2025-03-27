#include <message_buider.h>
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>


char* construct_message(const char *event, const char *state, const char *user, const char *password, char *out_buffer, size_t buffer_size) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "event", event);
    cJSON_AddStringToObject(root, "state", state);
    cJSON_AddStringToObject(root, "user", user);
    cJSON_AddStringToObject(root, "password", password);

    if (!cJSON_PrintPreallocated(root, out_buffer, buffer_size, 0)) {
        ESP_LOGE("MessageBuilder", "Failed to print JSON into buffer");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON_Delete(root);
    return out_buffer;
}