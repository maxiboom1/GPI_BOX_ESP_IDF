#include "eth_setup.h"
#include "web_server.h"
#include "spiffs_setup.h"
#include "esp_log.h"
#include "app_config.h"

void app_main(void)
{
	ESP_ERROR_CHECK(init_config());
    ESP_ERROR_CHECK(load_config());
    
    ESP_ERROR_CHECK(init_ethernet_static());
    
    ESP_ERROR_CHECK(init_spiffs());  
    
    ESP_ERROR_CHECK(start_webserver());
    
    ESP_LOGI("MAIN", "Loaded Config:");

	ESP_LOGI("MAIN", "Serial Enabled: %d", globalConfig.serialEnabled);
	ESP_LOGI("MAIN", "Admin Password: %s", globalConfig.adminPassword);


}