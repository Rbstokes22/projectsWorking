#ifndef AS7341_LIBRARY_HPP
#define AS7341_LIBRARY_HPP

#include "stdint.h"
#include "I2C/I2C.hpp"

namespace AS7341_DRVR {

enum class REG : uint8_t {
    ENABLE = 0x80, 
    CONFIG = 0x70,
    LED = 0x74,
    ATIME = 0x81,
    ASTEP_LWR = 0xCA,
    ASTEP_UPR = 0xCB,
    WTIME = 0x83,

};

enum class LED : uint8_t {OFF, ON};

// ASTEP 0 - 65534. (Recommended 599)
// ATIME 0 - 255. (Recommended 29)
// WTIME 0 - 255. (Recommend 0 or 1)
// Integration time is (ATIME + 1) * (ASTEP + 1) * WTIME
struct CONFIG {
    uint16_t ASTEP;
    uint8_t ATIME;
    uint8_t WTIME;
    bool LEDenable;
    CONFIG(
        uint16_t ASTEP = 599, 
        uint8_t ATIME = 29, 
        uint8_t WTIME = 0, 
        bool LEDenable = false
        );
};

class AS7341basic {
    private:
    i2c_master_dev_handle_t i2cHandle;
    bool safeData;
    CONFIG &conf;
    bool prepRegister(REG reg);
    void writeRegister(REG reg, uint8_t val);
    uint8_t readRegister(REG reg);
    void validateWrite(REG reg, uint8_t dataOut, bool verbose = true);

    public:
    AS7341basic(CONFIG &conf);
    bool init(uint8_t address);
    void led(LED state);

};

}

#endif // AS7341_HPP