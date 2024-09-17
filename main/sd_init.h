#ifndef SD_INIT_H
#define SD_INIT_H

#include "esp_err.h"
#include "driver/gpio.h"

#define SD_MISO_PIN     GPIO_NUM_13
#define SD_MOSI_PIN     GPIO_NUM_11
#define SD_CLK_PIN      GPIO_NUM_12
#define SD_CS_PIN       GPIO_NUM_10

esp_err_t init_sd_card(void);

#endif // SD_INIT_H