#include "tcp_client.h"
#include "app_config.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>
#include <unistd.h>
#include "freertos/task.h"

#define TAG "TCP_CLIENT"

extern AppConfig globalConfig;
static int tcp_socket = -1;
static TaskHandle_t tcp_task = NULL;
static TcpClientMode client_mode;

static void tcp_client_task(void *arg) {
    
    // Init config
    struct sockaddr_in dest_addr;
    
    // Based on mode (regular/companion) - set the ip/port
    if (client_mode == TCP_MODE_COMPANION) {
	    dest_addr.sin_addr.s_addr = globalConfig.companionIp;
	    dest_addr.sin_port = htons(globalConfig.companionPort);
	} else {
	    dest_addr.sin_addr.s_addr = globalConfig.tcpIp;
	    dest_addr.sin_port = htons(globalConfig.tcpPort);
	}
	
    dest_addr.sin_family = AF_INET;

    while (1) {
        // Close task (tcp_client) if disabled in config
        if (!globalConfig.tcpEnabled && !globalConfig.companionMode) {
            ESP_LOGW(TAG, "TCP and Companion disabled. Closing socket.");
            break;
        }

        tcp_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        
        // Cannot create socket, retry in 2 sec
        if (tcp_socket < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, "Connecting to %s:%d...", inet_ntoa(dest_addr.sin_addr), ntohs(dest_addr.sin_port));
        
        // If connection failed, reopen socket and retry in 2 sec
        if (connect(tcp_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
            ESP_LOGE(TAG, "Socket connect failed: errno %d", errno);
            close(tcp_socket);
            tcp_socket = -1;
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        ESP_LOGI(TAG, "TCP connected.");

        char rx_buffer[128];
        // If we are here - we are connected and listenings in loop
        while (1) {
            
            // If user set tcp and companion to off while we connected - exit
            if (!globalConfig.tcpEnabled && !globalConfig.companionMode) {
                ESP_LOGW(TAG, "TCP/Companion disabled. Closing socket.");
                break;
            }
            int len = recv(tcp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Receive failed: errno %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGW(TAG, "Connection closed by peer");
                break;
            } else {
                rx_buffer[len] = 0;
                ESP_LOGI(TAG, "Received: %s", rx_buffer);
                // TODO: parse GPO command here later
            }
        }

        close(tcp_socket);
        tcp_socket = -1;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    tcp_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t start_tcp_client_service(TcpClientMode mode) {
    if (tcp_task) return ESP_OK;
    client_mode = mode;
    return xTaskCreate(tcp_client_task, "tcp_client_task", 4096, NULL, 5, &tcp_task) == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t stop_tcp_client_service(void) {
    if (tcp_task) {
        vTaskDelete(tcp_task);
        tcp_task = NULL;
    }
    if (tcp_socket != -1) {
        close(tcp_socket);
        tcp_socket = -1;
    }
    return ESP_OK;
}

esp_err_t tcp_client_send(const char *json_data) {
    if (tcp_socket < 0) {
        ESP_LOGW(TAG, "Socket not connected");
        return ESP_FAIL;
    }

    int sent = send(tcp_socket, json_data, strlen(json_data), 0);
    if (sent < 0) {
        ESP_LOGE(TAG, "Send failed: errno %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TCP message sent");
    return ESP_OK;
}
