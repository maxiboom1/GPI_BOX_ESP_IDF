#include "eth_setup.h"
#include "web_server.h"
#include "spiffs_setup.h"
#include "esp_log.h"
#include "app_config.h"
#include "message_builder.h"
#include <http_client.h>
#include "freertos/idf_additions.h"
#include "tcp_client.h"
#include "gpio_handler.h"
#include "esp_netif.h"
#include "esp_netif_types.h"

// Forward declarations
void test_debug(void);
void wait_for_eth_ready(void);

void app_main(void)
{
	ESP_ERROR_CHECK(init_config()); //On each load - the nvs storage must be initialized
    ESP_ERROR_CHECK(load_config()); //Once the initialization is done - we can load the config to globalConfig
    
    ESP_ERROR_CHECK(init_ethernet_static()); //Initializes W5500 with static IP using values from globalConfig
    
	wait_for_eth_ready(); // Waits until IP is assigned and Ethernet is fully up.
    handle_config_change(); // This determines if tcp_client_task needs to be started (regular or Companion mode)
	ESP_ERROR_CHECK(init_spiffs()); // Mounts /spiffs_data partition, where HTML/JS/CSS is located.
    ESP_ERROR_CHECK(init_gpio_pins()); // Initializes all GPI pins with ISR/debounce, and GPO pins as outputs
    ESP_ERROR_CHECK(start_webserver()); // Starts HTTP server, sets up routes for /, /save, etc.
    
    ESP_LOGI("MAIN", "Loaded Config:");

    //test_debug(); // manual debug helper
}

void wait_for_eth_ready() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("ETH_DEF");

    while (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK || ip_info.ip.addr == 0) {
        ESP_LOGW("NET_WAIT", "Waiting for Ethernet to be ready...");
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    ESP_LOGI("NET_WAIT", "Ethernet is ready with IP: " IPSTR, IP2STR(&ip_info.ip));
}

// Utility debug
void test_debug(void){
	// *******Example for delay (must include freertos/idf_additions.h header)*******
	//#include "freertos/idf_additions.h" // this for delay use
	vTaskDelay(pdMS_TO_TICKS(5000));  // Wait 1 seconds
	
	// *******Example for message builder trigger*******
	char msg[256];
	construct_message("gpioChange", "HIGH","admin","password", msg, sizeof(msg));
	send_http_post(msg);
	
}
