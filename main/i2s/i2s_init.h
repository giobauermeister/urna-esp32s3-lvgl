#ifndef I2S_INIT_H
#define I2S_INIT_H

#include "driver/i2s_std.h"
#include "driver/i2s_types.h"
#include "driver/i2s_common.h"

#define I2S_SAMPLE_RATE (44100)
#define I2S_BITS_PER_SAMPLE I2S_DATA_BIT_WIDTH_16BIT
#define I2S_CHANNEL_MODE I2S_SLOT_MODE_MONO

// Channel handle
extern i2s_chan_handle_t tx_channel;

void i2s_init(void);

#endif  // I2S_INIT_H