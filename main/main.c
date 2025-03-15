#include "eth_setup.h"
#include "web_server.h"
#include "spiffs_setup.h"
#include "esp_log.h"

void app_main(void)
{
    ESP_ERROR_CHECK(init_ethernet_static());
    
    ESP_ERROR_CHECK(init_spiffs());  
    
    ESP_ERROR_CHECK(start_webserver());

    ESP_LOGI("MAIN", "Ethernet configured with static IP");
}