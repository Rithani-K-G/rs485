
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
