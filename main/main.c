#include "eth_setup.h"
#include "web_server.h"
#include "spiffs_setup.h"
#include "esp_log.h"
#include "app_config.h"
#include <message_buider.h>
#include <http_client.h>
#include "freertos/idf_additions.h"

// Forward declaration for debug utility
void test_debug(void);

void app_main(void)
{
	ESP_ERROR_CHECK(init_config());
    ESP_ERROR_CHECK(load_config());
    
    ESP_ERROR_CHECK(init_ethernet_static());
    
    ESP_ERROR_CHECK(init_spiffs());  
    
    ESP_ERROR_CHECK(start_webserver());
    
    ESP_LOGI("MAIN", "Loaded Config:");
	test_debug();
}

// Utility debug
void test_debug(void){
	
	// *******Example for delay (must include freertos/idf_additions.h header)*******
	//#include "freertos/idf_additions.h" // this for delay use
	vTaskDelay(pdMS_TO_TICKS(5000));  // Wait 1 seconds
	
	
	// *******Example for message builder trigger*******
	char msg[256];
	construct_message("gpioChange", "HIGH", msg, sizeof(msg));
	send_http_post(msg);
	
	
}


