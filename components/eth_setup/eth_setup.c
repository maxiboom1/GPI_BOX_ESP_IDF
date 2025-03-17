#include "eth_setup.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_eth_netif_glue.h"
#include "ethernet_init.h"
#include "lwip/ip_addr.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "app_config.h"  

static const char *TAG = "ETH_SETUP";

esp_err_t init_ethernet_static(void)
{
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

    //Load IP settings from globalConfig
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = globalConfig.deviceIp;
    ip_info.gw.addr = globalConfig.gateway;
    ip_info.netmask.addr = globalConfig.subnetMask;

    //Apply static IP
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));

    // Start Ethernet
    ESP_ERROR_CHECK(esp_eth_start(eth_handles[0]));

    ESP_LOGI(TAG, "Ethernet configured with IP: " IPSTR ", Gateway: " IPSTR ", Mask: " IPSTR,
             IP2STR(&ip_info.ip), IP2STR(&ip_info.gw), IP2STR(&ip_info.netmask));

    return ESP_OK;
}
