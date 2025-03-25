#pragma once

#include "esp_err.h"

esp_err_t init_ethernet_static(void);
esp_err_t reapply_eth_config(void);
