#pragma once

#include <esp_err.h>

esp_err_t send_http_post(const char *json_data);