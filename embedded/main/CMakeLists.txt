idf_component_register(SRCS "hello_world_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_driver_gpio nvs_flash esp_wifi esp_event esp_netif esp32-camera esp_http_server mqtt
                       PRIV_REQUIRES spi_flash)