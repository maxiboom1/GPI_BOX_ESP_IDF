idf_component_register(SRCS "web_server.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_server spiffs app_config eth_setup json)