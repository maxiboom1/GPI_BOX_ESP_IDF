idf_component_register(SRCS "eth_setup.c"
                       INCLUDE_DIRS "."
                       REQUIRES ethernet_init esp_eth esp_netif esp_event app_config)
