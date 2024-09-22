#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"

namespace I2C {

enum class I2C_RET {
    INIT_OK, INIT_FAIL, RUNNING
};

I2C_RET i2c_master_init(uint32_t frequency);
i2c_device_config_t configDev(uint8_t i2cAddr);
i2c_master_dev_handle_t addDev(i2c_device_config_t &dev_cfg);
}

#endif // I2C_HPP