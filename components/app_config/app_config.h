#pragma once

#include "esp_err.h"

// Configuration structure
typedef struct {
    uint32_t deviceIp;
    uint32_t gateway;
    uint32_t subnetMask;
    uint32_t companionIp;
    uint16_t companionPort;
    uint8_t  companionMode;
    uint8_t tcpEnabled;
    uint32_t tcpIp;
    uint16_t tcpPort;
    uint8_t tcpSecure;
    char tcpUser[32];
    char tcpPassword[32];
    uint8_t httpEnabled;
    char httpUrl[64];
    uint8_t httpSecure;
    char httpUser[32];
    char httpPassword[32];
    uint8_t serialEnabled;
    char adminPassword[32];
    uint8_t configFlag;
} AppConfig;

// Global Config Instance
extern AppConfig globalConfig;

// Function Declarations
esp_err_t init_config(void);
esp_err_t load_config(void);
esp_err_t save_config(void);
void set_default_config(void);
void handle_config_change(void);