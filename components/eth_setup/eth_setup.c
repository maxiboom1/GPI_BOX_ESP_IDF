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
static esp_netif_t *eth_netif = NULL;
static esp_eth_handle_t eth_handle = NULL;

esp_err_t init_ethernet_static(void)
{
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles = NULL;

    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));
    eth_handle = eth_handles[0];  // Store the first handle

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);  // Store netif

    esp_eth_netif_glue_handle_t eth_netif_glue = esp_eth_new_netif_glue(eth_handle);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, eth_netif_glue));

    esp_netif_ip_info_t ip_info = {
        .ip.addr = globalConfig.deviceIp,
        .gw.addr = globalConfig.gateway,
        .netmask.addr = globalConfig.subnetMask
    };

    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG, "Ethernet configured with IP: " IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

esp_err_t reapply_eth_config(void)
{
    if (!eth_netif) return ESP_ERR_INVALID_STATE;

    esp_netif_dhcp_status_t dhcp_status;
    if (esp_netif_dhcpc_get_status(eth_netif, &dhcp_status) == ESP_OK &&
        dhcp_status == ESP_NETIF_DHCP_STARTED) {
        esp_netif_dhcpc_stop(eth_netif);  // stop only if active
    }

    esp_netif_ip_info_t ip_info = {
        .ip.addr = globalConfig.deviceIp,
        .gw.addr = globalConfig.gateway,
        .netmask.addr = globalConfig.subnetMask
    };

    ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));

    ESP_LOGI(TAG, "Ethernet IP updated to: " IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}