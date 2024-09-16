#include "i2c_init.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

i2c_master_dev_handle_t pcf8574_i2c_device_handle = NULL;

esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t i2c_master_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_cfg, &bus_handle));

    i2c_device_config_t pcf8574_i2c_device_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PCF8574_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &pcf8574_i2c_device_cfg, &pcf8574_i2c_device_handle));

    return ESP_OK;
}

