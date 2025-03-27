#pragma once
#include "esp_err.h"

typedef enum {
    TCP_MODE_REGULAR,
    TCP_MODE_COMPANION
} TcpClientMode;

esp_err_t start_tcp_client_service(TcpClientMode mode);
esp_err_t stop_tcp_client_service(void);
esp_err_t tcp_client_send(const char *json_data);