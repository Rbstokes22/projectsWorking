#ifndef AS7341_LIBRARY_HPP
#define AS7341_LIBRARY_HPP

#include "stdint.h"
#include "I2C/I2C.hpp"

namespace AS7341_DRVR {

enum class REG : uint8_t {
    ENABLE = 0x80, 
    CONFIG = 0x70

};

class AS7341basic {
    private:
    i2c_master_dev_handle_t i2cHandle;
    void writeRegister(REG reg, uint8_t val);
    void readRegister(REG reg, uint8_t* data, size_t size);

    public:
    AS7341basic();
    bool init(uint8_t address);

};

}

#endif // AS7341_HPP