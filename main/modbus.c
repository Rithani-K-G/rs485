/* #include "modbus.h"
#include "mbcontroller.h"   // esp-modbus master API
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "MODBUS";

// Handle returned by the constructor — every subsequent esp-modbus
// call needs this, so it stays local to this file (main.c never sees it).
static void *master_handle = NULL;

// --- Parameter table -------------------------------------------------
// Each row here maps one "characteristic" (a named value you care about)
// to a specific register on a specific slave. This is a placeholder —
// once you have the slave device's register map (from its datasheet),
// replace CID_TEST_REG with real entries: one row per register you
// actually want to poll.
enum {
    CID_TEST_REG = 0,
};

// Where the master stores the value it just read for each CID.
// Instance offset in the table below points into this struct.
typedef struct {
    uint16_t test_reg;
} modbus_values_t;

static modbus_values_t values;
static const mb_parameter_descriptor_t param_table[] = {
    { CID_TEST_REG,  "test_reg",  "raw",  1,
      MB_PARAM_INPUT, 0,     1,
      offsetof(modbus_values_t, test_reg),       // where to store the value
      PARAM_TYPE_U16, 2,
      { .opt1 = 0, .opt2 = 0, .opt3 = 0 },
      PAR_PERMS_READ_TRIGGER },
};


esp_err_t modbus_master_init(int uart_port, int baud_rate,
                              int tx_pin, int rx_pin, int rts_pin,
                              uint8_t slave_addr)
{
    // Step 1: describe the serial link and hand it to the constructor.
    // This creates the underlying UART driver internally.
    mb_communication_info_t comm = {
        .ser_opts.port = uart_port,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = baud_rate,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
        .ser_opts.uid = 0,                 // unused on the master side
        .ser_opts.response_tout_ms = 1000, // how long to wait for a slave reply
    };

    esp_err_t err = mbc_master_create_serial(&comm, &master_handle);
    if (err != ESP_OK || master_handle == NULL) {
        ESP_LOGE(TAG, "mb controller create failed (0x%x)", err);
        return err;
    }

    // Step 2: RS485 is half-duplex over a transceiver (e.g. MAX485), so
    // the UART driver needs to know which GPIO drives the transceiver's
    // DE/RE enable pin. UART_MODE_RS485_HALF_DUPLEX makes the driver
    // toggle that pin automatically around each transmission — you
    // never touch it by hand in application code.
    ESP_ERROR_CHECK(uart_set_pin(uart_port, tx_pin, rx_pin,
                                  rts_pin, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(uart_port, UART_MODE_RS485_HALF_DUPLEX));

    // Step 3: fix up the slave address in the table (kept as a function
    // argument rather than hardcoded, so main.c controls it like your
    // other #defines) and load the table into the controller.
    mb_parameter_descriptor_t *table = (mb_parameter_descriptor_t *)param_table;
    table[CID_TEST_REG].mb_slave_addr = slave_addr;

    ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handle, param_table,
                                               sizeof(param_table) / sizeof(param_table[0])));

    // Step 4: everything above only configures the object — nothing is
    // sent on the wire until start is called.
    err = mbc_master_start(master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mb controller start failed (0x%x)", err);
        return err;
    }

    ESP_LOGI(TAG, "Modbus master started on UART%d, slave addr %d", uart_port, slave_addr);
    return ESP_OK;
}

esp_err_t modbus_master_poll(void)
{
    const mb_parameter_descriptor_t *param = NULL;
    uint8_t type = 0;

    // mbc_master_get_parameter() sends the actual request, blocks for
    // the slave's response (up to response_tout_ms), and copies the
    // result into the offset you gave it in modbus_values_t.
    esp_err_t err = mbc_master_get_parameter(master_handle, CID_TEST_REG,
                                              (uint8_t *)&values.test_reg, &type);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "test_reg = %u", values.test_reg);
    } else {
        ESP_LOGW(TAG, "read failed for test_reg (0x%x)", err);
    }
    return err;
}
    */

#include "modbus.h"
#include "mbcontroller.h"   // esp-modbus (FreeModbus-based) master API
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "MODBUS_MASTER";

// Session handle -- every esp-modbus call after init needs this.
// Stays private to this file, same as before.
static void *master_handle = NULL;

// One CID per value you care about. Right now that's just the single
// energy reading coming off ModSim. Add more rows here later when the
// real meter's register map (voltage, current, kWh, etc.) is known.
enum {
    CID_ENERGY_VALUE = 0,
};

// Where the master stores what it reads back. offsetof() below tells
// esp-modbus exactly which byte offset in this struct to write into
// for each CID -- this is how one struct can hold many different
// register values later without you writing separate read functions.
typedef struct {
    uint16_t energy_value;
} modbus_values_t;

static modbus_values_t values;

// mb_slave_addr is set at runtime in modbus_master_init() from the
// slave_addr argument, so it isn't hardcoded here.
static mb_parameter_descriptor_t param_table[] = {
    { CID_ENERGY_VALUE, "energy_value", "raw", 0,     // mb_slave_addr filled in below
      MB_PARAM_INPUT,        // ModSim test value = holding register (function code 03)
      0,                       // register address 0 (= "40001" in ModSim's display numbering)
      1,                       // 1 register wide (16-bit int, matches "int" energy value for now)
      offsetof(modbus_values_t, energy_value),
      PARAM_TYPE_U16, 2,
      { .opt1 = 0, .opt2 = 0, .opt3 = 0 },
      PAR_PERMS_READ_TRIGGER },
};

esp_err_t modbus_master_init(int uart_port, int baud_rate,
                              int tx_pin, int rx_pin, int rts_pin,
                              uint8_t slave_addr)
{
    // Step 1: describe the serial link -- this internally creates the UART driver.
    mb_communication_info_t comm = {
        .ser_opts.port = uart_port,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = baud_rate,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
        .ser_opts.uid = 0,                 // unused on the master side
        .ser_opts.response_tout_ms = 1000, // how long to wait for ModSim's reply
    };

    esp_err_t err = mbc_master_create_serial(&comm, &master_handle);
    if (err != ESP_OK || master_handle == NULL) {
        ESP_LOGE(TAG, "mb controller create failed (0x%x)", err);
        return err;
    }

    // Step 2: RS485 is half-duplex -- one pair of wires, either
    // transmitting or receiving, never both. rts_pin drives the
    // transceiver's DE/RE line; the driver toggles it automatically
    // around each transmission once this mode is set.
    ESP_ERROR_CHECK(uart_set_pin(uart_port, tx_pin, rx_pin,
                                  rts_pin, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(uart_port, UART_MODE_RS485_HALF_DUPLEX));

    // Step 3: the slave address must match whatever Slave ID you set
    // in ModSim -- this is how the request on the shared bus gets
    // answered by the right device (matters more once you have
    // multiple slaves on the same line).
    param_table[CID_ENERGY_VALUE].mb_slave_addr = slave_addr;

    ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handle, param_table,
                                               sizeof(param_table) / sizeof(param_table[0])));

    // Step 4: nothing is sent on the wire until start is called.
    err = mbc_master_start(master_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mb controller start failed (0x%x)", err);
        return err;
    }

    ESP_LOGI(TAG, "Modbus master started on UART%d, slave addr %d", uart_port, slave_addr);
    return ESP_OK;
}

esp_err_t modbus_master_read_energy(int *out_value)
{
    const mb_parameter_descriptor_t *param = NULL; // unused output, required by the API
    uint8_t type = 0;

    // This is the actual request: sends "read holding register 0" over
    // RS485, blocks until ModSim replies (or response_tout_ms expires),
    // and lands the result straight into values.energy_value.
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
