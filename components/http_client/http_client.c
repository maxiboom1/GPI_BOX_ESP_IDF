#include "http_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "app_config.h"

#define TAG "HTTP_CLIENT"
#define DEFAULT_HTTP_PORT 80
#define MAX_MSG_SIZE 256

extern AppConfig globalConfig;

typedef struct {
    char host[32];
    uint16_t port;
    char route[64];
} UrlParts;

static esp_err_t parse_url(const char *url, UrlParts *parts) {
    const char *start = strstr(url, "://");
    if (!start) return ESP_FAIL;
    start += 3;

    const char *host_end = start;
    while (*host_end && *host_end != ':' && *host_end != '/') host_end++;

    size_t host_len = host_end - start;
    if (host_len >= sizeof(parts->host)) return ESP_FAIL;
    strncpy(parts->host, start, host_len);
    parts->host[host_len] = '\0';

    parts->port = DEFAULT_HTTP_PORT;
    if (*host_end == ':') {
        host_end++;
        const char *port_start = host_end;
        while (*host_end && *host_end != '/') host_end++;
        char port_str[6] = {0};
        size_t port_len = host_end - port_start;
        if (port_len >= sizeof(port_str)) return ESP_FAIL;
        strncpy(port_str, port_start, port_len);
        parts->port = atoi(port_str);
    }

    if (*host_end == '/') {
        strncpy(parts->route, host_end, sizeof(parts->route) - 1);
        parts->route[sizeof(parts->route) - 1] = '\0';
    } else {
        strcpy(parts->route, "/");
    }

    return ESP_OK;
}

static void tcp_post_task(void *arg) {
    
    char *json_data = (char *)arg;
    UrlParts parts = {0};
	int sock = -1;  // Initialize here
    if (parse_url(globalConfig.httpUrl, &parts) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to parse URL: %s", globalConfig.httpUrl);
        goto cleanup;
    }
    char msg[MAX_MSG_SIZE];
    
    int len = snprintf(
        msg, sizeof(msg),
        "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
        parts.route, 
        parts.host, 
        strlen(json_data), 
        json_data
    );
    
    if (len >= sizeof(msg)) {
        ESP_LOGE(TAG, "Message too long for buffer");
        goto cleanup;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed");
        goto cleanup;
    }

    fcntl(sock, F_SETFL, O_NONBLOCK);

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(parts.port);
    dest.sin_addr.s_addr = inet_addr(parts.host);

    // Start non-blocking connect
    int ret = connect(sock, (struct sockaddr *)&dest, sizeof(dest));
    if (ret < 0 && errno != EINPROGRESS) {
        ESP_LOGE(TAG, "Connect failed: %d", errno);
        goto cleanup;
    }

    // Wait briefly for connection to complete (fire-and-forget style)
    struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };  // 100 ms timeout
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sock, &writefds);
    if (select(sock + 1, NULL, &writefds, NULL, &tv) > 0) {
        if (FD_ISSET(sock, &writefds)) {
            // Connection established, send data
            if (send(sock, msg, len, 0) < 0) {
                ESP_LOGE(TAG, "Send failed: %d", errno);
            }
        }
    } else {
        ESP_LOGW(TAG, "Connection not ready in time");
    }
	ESP_LOGI(TAG, "Stack high watermark: %d", uxTaskGetStackHighWaterMark(NULL));
    cleanup:
        if (sock >= 0) close(sock);
        free(json_data);
        vTaskDelete(NULL);
}

esp_err_t send_http_post(const char *json_data) {
    char *json_copy = strdup(json_data);
    if (!json_copy) {
        ESP_LOGE(TAG, "Failed to allocate memory for JSON");
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(tcp_post_task, "tcp_post_task", 2048, json_copy, 5, NULL) != pdPASS) {
        free(json_copy);
        ESP_LOGE(TAG, "Task creation failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}