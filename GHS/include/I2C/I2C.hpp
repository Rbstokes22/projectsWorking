#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"
#include "UI/MsgLogHandler.hpp"

namespace Serial {

#define I2C_LOG_METHOD Messaging::Method::SRL_LOG

enum class I2C_FREQ : uint32_t {STD = 100000, FAST = 400000};

class I2C {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    I2C_FREQ freq; // Frequency, STA 100kHz, FAST 400kHz.
    i2c_master_bus_handle_t busHandle; // i2c bus handle
    bool isInit; // Is initialized.
    I2C();
    I2C(const I2C&) = delete; // prevent copying
    I2C &operator=(const I2C&) = delete; // prevent assignment
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::CRITICAL, bool ignoreRepeat = false);

    public:
    static I2C* get();
    bool i2c_master_init(I2C_FREQ freq = I2C_FREQ::STD);
    i2c_device_config_t configDev(uint8_t i2cAddr);
    i2c_master_dev_handle_t addDev(i2c_device_config_t &dev_cfg);
};

}

#endif // I2C_HPP