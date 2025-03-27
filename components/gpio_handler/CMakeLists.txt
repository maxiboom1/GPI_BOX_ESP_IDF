idf_component_register(SRCS "gpio_handler.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver app_config message_builder tcp_client http_client esp_timer)