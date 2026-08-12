/*
#ifndef MODBUS_H
#define MODBUS_H
#define MB_UART_PORT   2
#define MB_BAUD_RATE   9600
#define MB_TX_PIN      17
#define MB_RX_PIN      16
#define MB_RTS_PIN     4
#define MB_SLAVE_ADDR  1

#include "esp_err.h"
#include <stdint.h>

// Sets up the RS485/Modbus RTU master: creates the controller object,
// configures the UART pins in RS485 half-duplex mode, loads the
// register table, and starts the stack. Call once from app_main().
esp_err_t modbus_master_init(int uart_port, int baud_rate,
                              int tx_pin, int rx_pin, int rts_pin,
                              uint8_t slave_addr);

// Runs one read cycle across all configured parameters and logs the
// results. Call this periodically (e.g. from a dedicated FreeRTOS task).
esp_err_t modbus_master_poll(void);

#endif */


#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "esp_err.h"
#include <stdint.h>

// One-time setup: opens the UART, configures RS485 half-duplex, loads
// the parameter table, starts the Modbus master task.
esp_err_t modbus_master_init(int uart_port, int baud_rate,
                              int tx_pin, int rx_pin, int rts_pin,
                              uint8_t slave_addr);

// Call this repeatedly (e.g. in your main loop) to request the latest
// energy value from the slave. Returns the read value via *out_value.
esp_err_t modbus_master_read_energy(int *out_value);

#endif
