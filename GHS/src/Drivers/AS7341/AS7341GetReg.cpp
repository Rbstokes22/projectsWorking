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

// Requires dataSafe bool. Returns uint16_t value along with
// dataSafe update.
uint16_t AS7341basic::getLEDCurrent(bool &dataSafe) {
    uint8_t raw = this->readRegister(REG::LED, dataSafe); 

    // Clears bits 0 - 6. Moves bit 7 to bit 0 to determine
    // if 1 (on) or 0 (off).
    uint8_t onBit = (raw & 0x80) >> 7; 

    // If on, extracts bits 0 - 6, and doubles them. Adds 
    // 4 to return proper milli amp value (4 - 258)
    if (onBit == 1) {
        return ((raw & 0x7F) << 1) + 4;
    }

    return 0; // returns if off show 0 milli Amps.
}

// Requires dataSafe bool. Returns uint8_t register data.
uint8_t AS7341basic::getAGAIN_RAW(bool &dataSafe) {
    return this->readRegister(REG::AGAIN, dataSafe);
}

// Requires dataSafe bool. Returns float from 0.5 to 512.
float AS7341basic::getAGAIN(bool &dataSafe) {

    // Gets the raw AGAIN data. Bits 0 - 4.
    uint8_t val = this->getAGAIN_RAW(dataSafe) & 0b00011111;

    // If value is 0 returns 0.5. If > 0, shifts bits by 
    // val - 1 and returns it as a float.
    // EX: val = 10, 0x01 << (10 - 1) = 512.
    if (val == 0) {
        return 0.5f;
    } else {
        return (0x01 << (val - 1)) * 1.0f;
    }
}

// Requires dataSafe bool. Returns uint16_t value along with
// dataSafe bool update.
uint16_t AS7341basic::getASTEP_RAW(bool &dataSafe) {
    bool lwr_safe{false}, upr_safe{false};

    uint8_t lower = this->readRegister(REG::ASTEP_LWR, lwr_safe);
    uint8_t upper = this->readRegister(REG::ASTEP_UPR, upr_safe);

    dataSafe = (lwr_safe && upr_safe);
    return ((upper << 8) | lower);
}

// Requires dataSafe bool. Returns ASTEP float in microseconds.
float AS7341basic::getASTEP(bool &dataSafe) {
    uint16_t val = this->getASTEP_RAW(dataSafe);
    return (val + 1) * 2.78f; // +1 is that datasheet computation.
}

// Requires dataSafe bool. Returns uint8_t value. There is no
// float return for ATIme like there is for the other methods.
uint8_t AS7341basic::getATIME_RAW(bool &dataSafe) {
    return this->readRegister(REG::ATIME, dataSafe);
}

// Requires dataSafe bool. Computes integration time by
// computing (ASTEP + 1) * (ATIME + 1) * 2.78µs. Returns
// float value.
float AS7341basic::getIntegrationTime(bool &dataSafe) {
    bool astep_safe{false}, atime_safe{false};

    uint16_t astep = this->getASTEP_RAW(astep_safe);
    uint8_t atime = this->getATIME_RAW(atime_safe);

    dataSafe = (astep_safe && atime_safe);
    return (astep + 1) * (atime + 1) * 2.78f;
}

// Requires dataSafe bool. Returns uint8_t wtime value.
uint8_t AS7341basic::getWTIME_RAW(bool &dataSafe) {
    return this->readRegister(REG::WTIME, dataSafe);
}

// Requires dataSafe bool. Returns float of wait time in millis.
float AS7341basic::getWTIME(bool &dataSafe) {
    return (this->getWTIME_RAW(dataSafe) + 1) * 2.78f;
}

// Requires dataSafe bool. Returns uint8_t value of SMUX register.
uint8_t AS7341basic::getSMUX(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::SMUX_CONFIG, dataSafe);
    data &= 0b00011000; // isolates bits 3 and 4.
    return (data >> 3); // returns value shifted 3. (0-3).
}

// Requires dataSafe bool. Returns true of false if enabled.
bool AS7341basic::getWaitEnabled(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::ENABLE, dataSafe);
    data &= 0b00001000; // isolates bit 3.
    return data >> 3; // retuns value shifted 3. (0 or 1).
}

// Requires dataSafe bool. Returns true of false if enabled.
bool AS7341basic::getSpectrumEnabled(bool &dataSafe) {
    uint8_t data = this->readRegister(REG::ENABLE, dataSafe);
    data &= 0b00000010; // isolates bit 1.
    return data >> 1; // returns value shifted 1. (0 or 1).
}

}