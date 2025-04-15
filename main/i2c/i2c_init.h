#ifndef I2C_INIT_H
#define I2C_INIT_H

#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_20
#define I2C_MASTER_SDA_IO           GPIO_NUM_19
#define I2C_MASTER_PORT_NUM         I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT          1000

#define PCF8574_ADDR                0x20

extern i2c_master_dev_handle_t pcf8574_i2c_device_handle;

esp_err_t i2c_master_init(void);

#endif // I2C_INIT_H