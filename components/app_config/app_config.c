#include "app_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "lwip/ip_addr.h"
#include <string.h>
#include "tcp_client.h"  

static const char *TAG = "APP_CONFIG";
AppConfig globalConfig;  // Define global config object

//Triggers on config load/save. Starts or stops TCP task based on mode.
void handle_config_change(void) {
    stop_tcp_client_service();
    ESP_LOGE(TAG, "Stopping tcp-service");
    vTaskDelay(pdMS_TO_TICKS(1500));  // delay before restarting
    if (globalConfig.tcpEnabled) {
        ESP_LOGE(TAG, "Start tcp-service as regular");
        start_tcp_client_service(TCP_MODE_REGULAR);
    } else if (globalConfig.companionMode) {
        ESP_LOGE(TAG, "Start tcp-service as companion");
        start_tcp_client_service(TCP_MODE_COMPANION);
    }
}

// Init the NVS storage that holds device config
esp_err_t init_config() {
    esp_err_t err = nvs_flash_init();

    // If NVS storage needs to be reflashed (duo size change for example) - we erase and re-init it.
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased, performing reset...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    // On error - show error
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;  // Return error if NVS init fails
    }

    // OK - NVS initialized
    ESP_LOGI("APP_CONFIG", "NVS initialized successfully");
    return ESP_OK;
}

//Load the config data from NVS to in-memory globalConfig
esp_err_t load_config(void) {
    nvs_handle_t nvs_handle; // This will hold the config data from NVS

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);  //Open NVS in READWRITE mode and copy it to nvs_handle

    // On Err
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace 'storage': %s", esp_err_to_name(err));
        return err;
    }

    size_t size = sizeof(AppConfig); // Get size of AppConfig struct

    err = nvs_get_blob(nvs_handle, "app_config", &globalConfig, &size); // Load blob from NVS into globalConfig ("app_config" is the key in namespace "storage")

    // If no data - load to globalConfig default values.
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "No config found in NVS, applying defaults...");
        set_default_config();   //Use globalConfig
        save_config();          //Save default config to NVS
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read config: %s", esp_err_to_name(err)); // Some error
    } else {
        ESP_LOGI(TAG, "Configuration loaded successfully."); // OK
    }

    nvs_close(nvs_handle); // Stop reading NVS and close the api
    return ESP_OK;
}

//Saves current globalConfig to NVS.
esp_err_t save_config(void) {
    
    nvs_handle_t nvs_handle;
    
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS storage for writing: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(nvs_handle, "app_config", &globalConfig, sizeof(AppConfig));
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle); // Commit save
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved successfully.");
        } else {
            ESP_LOGE(TAG, "Failed to commit config: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to set config blob: %s", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    handle_config_change(); // Once app configuration changed - call change handler func.
    return err;
}

//Set default values to globalConfig, save in to NVS, save triggers change handler func.
void set_default_config(void) {
    globalConfig.deviceIp = ipaddr_addr("10.168.0.177");
    globalConfig.gateway = ipaddr_addr("10.168.0.1");
    globalConfig.subnetMask = ipaddr_addr("255.255.255.0");

    globalConfig.companionIp = ipaddr_addr("0.0.0.0");
    globalConfig.companionPort = 9567;
    globalConfig.companionMode = 0;
    
    globalConfig.tcpEnabled = 0;
    globalConfig.tcpIp = ipaddr_addr("0.0.0.0");
    globalConfig.tcpPort = 0;
    globalConfig.tcpSecure = 0;
    memset(globalConfig.tcpUser, 0, sizeof(globalConfig.tcpUser));
    memset(globalConfig.tcpPassword, 0, sizeof(globalConfig.tcpPassword));
    globalConfig.httpEnabled = 0;
    memset(globalConfig.httpUrl, 0, sizeof(globalConfig.httpUrl));
    globalConfig.httpSecure = 0;
    memset(globalConfig.httpUser, 0, sizeof(globalConfig.httpUser));
    memset(globalConfig.httpPassword, 0, sizeof(globalConfig.httpPassword));
    globalConfig.serialEnabled = 0;
    strncpy(globalConfig.adminPassword, "admin", sizeof(globalConfig.adminPassword));
    globalConfig.configFlag = 0xAA;

    ESP_LOGI(TAG, "Default configuration applied.");
    save_config();
}
