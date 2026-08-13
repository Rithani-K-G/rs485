idf_component_register(SRCS "eth.c" "modbus_tcp.c" "modbus_rtu.c" "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                    esp_driver_gpio
                    esp_driver_spi
                    esp_eth                         # base Ethernet driver framework (esp_eth.h, esp_eth_driver_install, etc.)
                    esp_netif
                    lwip
                    nvs_flash
                    esp_event )
