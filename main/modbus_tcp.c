#include "modbus_tcp.h"
#include "mbcontroller.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "MODBUS_MASTER";
static void *master_handle = NULL;

enum {
    CID_ENERGY_VALUE = 0,
};

// Same idea as the RTU version -- this struct is where the read value lands.
typedef struct {
    uint16_t energy_value;
} modbus_values_t;

static modbus_values_t values;

// Table shape is identical to the RTU version. Only the *meaning* of
// mb_slave_addr changes -- see the .h comment. Everything else (CID,
// register type, address, offset) still means the same thing.
static mb_parameter_descriptor_t param_table[] = {
    { CID_ENERGY_VALUE, "energy_value", "raw", 0,
      MB_PARAM_HOLDING,
      0,
      1,
      offsetof(modbus_values_t, energy_value),
      PARAM_TYPE_U16, 2,
      { .opt1 = 0, .opt2 = 0, .opt3 = 0 },
      PAR_PERMS_READ_TRIGGER },
};

// esp-modbus wants a NULL-terminated array of IP strings, not a single
// address -- because TCP master can talk to multiple slaves, each one
// indexed by its position in this array (matched against mb_slave_addr
// in the table above). We only have one slave, so it's a 1-entry table.
static char *slave_ip_table[2];

esp_err_t modbus_master_init(esp_netif_t *eth_netif, const char *slave_ip,
                              uint16_t tcp_port, uint8_t slave_addr)
{
    slave_ip_table[0] = (char *)slave_ip;
    slave_ip_table[1] = NULL;   // terminator -- required by the stack

    mb_communication_info_t comm = {
    .tcp_opts.mode = MB_TCP,
    .tcp_opts.addr_type = MB_IPV4,
    .tcp_opts.port = tcp_port,
    .tcp_opts.ip_addr_table = (void *)slave_ip_table,
    .tcp_opts.ip_netif_ptr = (void *)eth_netif,
};

    // TCP equivalent of mbc_master_create_serial() -- opens a TCP
    // client, no UART/pin config involved at all.
    esp_err_t err = mbc_master_create_tcp(&comm, &master_handle);
    if (err != ESP_OK || master_handle == NULL) {
        ESP_LOGE(TAG, "mb controller create failed (0x%x)", err);
        return err;
    }

    param_table[CID_ENERGY_VALUE].mb_slave_addr = slave_addr;

    ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handle, param_table,
                                               sizeof(param_table) / sizeof(param_table[0])));

    err = mbc_master_start(master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mb controller start failed (0x%x)", err);
        return err;
    }

    ESP_LOGI(TAG, "Modbus TCP master started, target %s:%d", slave_ip, tcp_port);
    return ESP_OK;
}

esp_err_t modbus_master_read_energy(int *out_value)
{
    uint8_t type = 0;

    // Same call as RTU -- this is the part that genuinely doesn't
    // change. esp-modbus hides the transport difference behind this
    // one function; TCP vs RTU only matters at init time.
    esp_err_t err = mbc_master_get_parameter(master_handle, CID_ENERGY_VALUE,
                                              (uint8_t *)&values.energy_value, &type);
    if (err == ESP_OK) {
        *out_value = (int)values.energy_value;
        ESP_LOGI(TAG, "energy_value = %d", *out_value);
    } else {
        ESP_LOGW(TAG, "read failed for energy_value (0x%x)", err);
    }
    return err;
}
