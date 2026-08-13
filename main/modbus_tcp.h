#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include "esp_err.h"
#include "esp_netif.h"
#include <stdint.h>

// eth_netif: the esp_netif_t* handle for your already-connected W5500
// interface. TCP Modbus attaches to this same interface, so Ethernet
// must already have an IP before this is called -- same rule as MQTT.
// slave_ip: IP address of the PC running ModSim / Modbus Slave app.
// tcp_port: Modbus TCP standard is 502.
// slave_addr: NOT a wire address here -- it's a 1-based index into the
// internal IP table (see modbus_master.c). With one slave, this is just 1.
esp_err_t modbus_master_init(esp_netif_t *eth_netif, const char *slave_ip,
                              uint16_t tcp_port, uint8_t slave_addr);

esp_err_t modbus_master_read_energy(int *out_value);

#endif
