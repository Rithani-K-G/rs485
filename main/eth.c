#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include "esp_eth_mac_w5500.h"
#include "esp_eth_phy_w5500.h"

static const char *TAG = "GATEWAY DEVICE";

#define ETH_SPI_HOST SPI2_HOST
#define PIN_SCK  18
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_CS   5
#define PIN_INT  21
#define PIN_RST  4

static void eth_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == ETHERNET_EVENT_CONNECTED) ESP_LOGI(TAG, "Ethernet Link Up");
    if (id == ETHERNET_EVENT_DISCONNECTED) ESP_LOGI(TAG, "Ethernet Link Down");
}

static void got_ip_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
}

static bool parse_ip_string(const char *ip_str, esp_ip4_addr_t *out_addr)
{
    unsigned int a, b, c, d;

    int matched = sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    if (matched != 4) {
        return false;
    }

    // 8 bit max 255
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return false;
    }

    out_addr->addr = ESP_IP4TOADDR(a, b, c, d);
    return true;
}

static esp_netif_t *eth_init(const char *ip_str, const char *gw_str)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

    esp_netif_ip_info_t ip_info;

    // Parse the IP passed in from main.c
    if (!parse_ip_string(ip_str, &ip_info.ip)) {
    ESP_LOGE(TAG, "Invalid IP string: %s", ip_str);
    return 0;
    }

    if (!parse_ip_string(gw_str, &ip_info.gw)) {
    ESP_LOGE(TAG, "Invalid gateway string: %s", gw_str);
    return 0;
    }

    ip_info.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);

    esp_netif_dhcpc_stop(eth_netif);
    esp_netif_set_ip_info(eth_netif, &ip_info);

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
        //new one line below
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = 1 * 1000 * 1000, // start slow (1MHz) for reliable bring-up
        .queue_size = 20,
        .spics_io_num = PIN_CS,
    };

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = PIN_INT;

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.reset_gpio_num = PIN_RST; // driver handles reset — no manual toggle needed

    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;

    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_handler, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    return eth_netif;       // added new
}

/*void ethernet(const char *ip_str, const char *gw_str)
{
    eth_init(ip_str, gw_str);
    vTaskDelay(pdMS_TO_TICKS(2000)); // let link come up
}*/ // newwwwwwwwwwwww

esp_netif_t *ethernet(const char *ip_str, const char *gw_str)
{
    esp_netif_t *eth_netif = eth_init(ip_str, gw_str);
    vTaskDelay(pdMS_TO_TICKS(2000)); // let link come up
    return eth_netif;
}
