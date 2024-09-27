#include "Drivers/AS7341/AS7341_Library.hpp"
#include "stdint.h"
#include "I2C/I2C.hpp"
#include "Config/config.hpp"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

namespace AS7341_DRVR {

uint16_t AS7341basic::getLEDCurrent(bool &dataSafe) {
    uint8_t raw = this->readRegister(REG::LED, dataSafe); 
    uint8_t onBit = (raw & 0x80) >> 7;

    if (onBit == 1) {
        return ((raw & 0x7F) << 1) + 4;
    }

    return 0;
}

uint8_t AS7341basic::getAGAIN_RAW(bool &dataSafe) {
    return this->readRegister(REG::AGAIN, dataSafe);
}

// Returns float from 0.5 to 512
float AS7341basic::getAGAIN(bool &dataSafe) {
    uint8_t val = this->getAGAIN_RAW(dataSafe);
    if (val == 0) {
        return 0.5f;
    } else {
        return (0x01 << (val - 1)) * 1.0f;
    }
}

uint16_t AS7341basic::getASTEP_RAW(bool &dataSafe) {
    bool lwr_safe{false}, upr_safe{false};

    uint8_t lower = this->readRegister(REG::ASTEP_LWR, lwr_safe);
    uint8_t upper = this->readRegister(REG::ASTEP_UPR, upr_safe);

    dataSafe = (lwr_safe && upr_safe);
    return ((upper << 8) | lower);
}

// Returns ASTEP in microseconds
float AS7341basic::getASTEP(bool &dataSafe) {
    uint16_t val = this->getASTEP_RAW(dataSafe);
    return (val + 1) * 2.78f;
}


uint8_t AS7341basic::getATIME_RAW(bool &dataSafe) {
    return this->readRegister(REG::ATIME, dataSafe);
}

// There is no float return for ATIME like there is for the
// other methods.

// Returns (ASTEP + 1) * (ATIME + 1) * 2.78µs.
float AS7341basic::getIntegrationTime(bool &dataSafe) {
    bool astep_safe{false}, atime_safe{false};

    uint16_t astep = this->getASTEP_RAW(astep_safe);
    uint8_t atime = this->getATIME_RAW(atime_safe);

    dataSafe = (astep_safe && atime_safe);
    return (astep + 1) * (atime + 1) * 2.78f;

}

uint8_t AS7341basic::getWTIME_RAW(bool &dataSafe) {
    return this->readRegister(REG::WTIME, dataSafe);
}

// Returns wait time in milliseconds.
float AS7341basic::getWTIME(bool &dataSafe) {
    return (this->getWTIME_RAW(dataSafe) + 1) * 2.78f;
}

uint8_t AS7341basic::getSMUX(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::SMUX_CONFIG, dataSafe);
    data &= 0b00011000;
    return (data >> 3);
}

bool AS7341basic::getWaitEnabled(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::ENABLE, dataSafe);
    data &= 0b00001000;
    return data >> 3;
}

bool AS7341basic::getSpectrumEnabled(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::ENABLE, dataSafe);
    data &= 0b00000010;
    return data >> 1;
}

}