
/*
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "modbus_rtu.h"
#define MB_UART_PORT   1
#define MB_BAUD_RATE   9600      // must match the baud rate set in ModSim's port config
#define MB_TX_PIN      17
#define MB_RX_PIN      16
#define MB_RTS_PIN     4
#define MB_SLAVE_ADDR  1         // must match Slave ID configured in ModSim

void app_main(void)
{
    modbus_master_init(MB_UART_PORT, MB_BAUD_RATE, MB_TX_PIN, MB_RX_PIN,
                        MB_RTS_PIN, MB_SLAVE_ADDR);

    int energy = 0;
    while (1) {
        modbus_master_read_energy(&energy);
        vTaskDelay(pdMS_TO_TICKS(2000));  // poll every 2s for now
    }
}
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "modbus_tcp.h"
#include "eth.h"
#include "esp_netif.h"   // for esp_netif_t
#include "esp_log.h"      // for ESP_LOGE

#define MB_SLAVE_IP    "192.168.54.89"        // IP of the PC running ModSim / Modbus Slave app
#define STATIC_IP   "192.168.54.205"
#define GATEWAY_IP  "192.168.54.1"
#define MB_TCP_PORT    502
#define MB_SLAVE_ADDR  1                // index into the IP table -- 1 with a single slave

static const char *TAG = "MODBUS TCP";

void app_main(void)
{
    // This is the one real dependency: eth_init() needs to hand back
    // the esp_netif_t* it creates, so Modbus can attach to the same
    // interface your MQTT code already uses. If your current eth_init()
    // is void, it needs a small change to return/expose that pointer.
    //esp_netif_t *eth_netif = eth_init();

    esp_netif_t *eth_netif = ethernet(STATIC_IP, GATEWAY_IP);
    if (eth_netif == NULL) {
    ESP_LOGE(TAG, "Ethernet init failed, halting");
    return;
    }

    // Whatever you already use to confirm Ethernet has a live IP
    // before touching MQTT -- same gate applies here.
    // wait_for_eth_connected();

    modbus_master_init(eth_netif, MB_SLAVE_IP, MB_TCP_PORT, MB_SLAVE_ADDR);

    int energy = 0;
    while (1) {
        modbus_master_read_energy(&energy);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
