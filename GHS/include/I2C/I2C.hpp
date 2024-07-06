#ifndef I2C_HPP
#define I2C_HPP

#include "config.hpp"
#include "driver/i2c_master.h"

namespace I2C {

bool i2c_master_init(uint32_t frequency);
i2c_device_config_t configDev(uint8_t i2cAddr);
i2c_master_dev_handle_t addDev(i2c_device_config_t &dev_cfg);
}

#endif // I2C_HPP