/*
#include "modbus.h"

void app_main(void)
{
    modbus_master_init(MB_UART_PORT, MB_BAUD_RATE, MB_TX_PIN, MB_RX_PIN, MB_RTS_PIN, MB_SLAVE_ADDR);
}
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "modbus.h"

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
