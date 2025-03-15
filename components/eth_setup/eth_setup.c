#include "eth_setup.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_eth_netif_glue.h" 
#include "ethernet_init.h"
#include "lwip/ip_addr.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "ETH_SETUP";

esp_err_t init_ethernet_static(void)
{
    esp_err_t ret = ESP_OK;
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles = NULL;

    // Initialize Ethernet driver
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));

    // Initialize TCP/IP stack and event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Ethernet interface
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    // Create glue to attach Ethernet driver to TCP/IP stack
    esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handles[0]);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, eth_netif_glue));

    // Static IP configuration
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 10, 168, 0, 177);
    IP4_ADDR(&ip_info.gw, 10, 168, 0, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));

    // Start Ethernet
    ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));

    ESP_LOGI("ETH_SETUP", "Ethernet configured with static IP: 10.168.0.177");

    return ESP_OK;
}
