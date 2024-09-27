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

bool AS7341basic::power(PWR state) {
    return this->validateWrite(REG::ENABLE, static_cast<uint8_t>(state));
}

// Requires value. Used to set integration time for spectral measurments.
// Determines how long the sensor collects light before taking the 
// measurements. Returns true or false. (Recommend 29).
bool AS7341basic::configATIME(uint8_t value) {
    return this->validateWrite(REG::ATIME, value);
}

// Requires value. Used to configure step time for SMUX operation. Controls
// timing of how long sensor waits between switching between different 
// photo diodes. Writes two, 8-bit registers equating to a 16-bit value.
// Returns true or false. (Recommend 599).
bool AS7341basic::configASTEP(uint16_t value) {
    uint8_t total{0};

    uint8_t ASTEPdata = 0x00 | (value & 0x00FF); // lower

    if (value >= 65535) value = 65534; // 65535 is reserved

    total += this->validateWrite(REG::ASTEP_LWR, ASTEPdata);

    ASTEPdata = 0x00 | (this->conf.ASTEP >> 8); // upper

    total += this->validateWrite(REG::ASTEP_UPR, ASTEPdata);
    return (total == 2);
}

// Requires value. Handles time between measurements. This contributes
// to the integration time using ASTEP and ATIME. Returns true or false.
bool AS7341basic::configWTIME(uint8_t value) {
    return this->validateWrite(REG::WTIME, value);
}

// Requires value. Writes to SMUX register 0 to init the SMUX, followed
// by set value 1 for Read SMUX config to RAM from SMUX chain, and 2 for 
// Write SMUX config from RAM to SMUX chain. Returns true or false.
bool AS7341basic::configSMUX(SMUX_CONF config) {
    static bool SMUX_INIT{false}; // Sent only once.
    bool dataSafe{false};
    uint8_t total{0};
    uint8_t smux_conf = this->readRegister(REG::SMUX_CONFIG, dataSafe);
    uint8_t init = smux_conf & 0b11100111; // Clear bits 3 and 4.
    uint8_t conf = smux_conf | (static_cast<uint8_t>(config) << 3); // Move value to bits 3 and 4.

    if (!SMUX_INIT && dataSafe) {
        total += this->validateWrite(REG::SMUX_CONFIG, init);
        SMUX_INIT = true;
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Short delay between sends.

    if (dataSafe) total += this->validateWrite(REG::SMUX_CONFIG, conf);

    return (total == 2);
}

bool AS7341basic::enableLED(LED_CONF state) {
    bool dataSafe{false};
    uint8_t configREG = this->readRegister(REG::CONFIG, dataSafe);

    switch (state) {
        case LED_CONF::ENABLE:
        configREG |= 0x08;
        break;

        case LED_CONF::DISABLE:
        configREG &= 0b11110111;
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::CONFIG, configREG);
    } else {
        return false;
    }
}

bool AS7341basic::enableSMUX(SMUX state) {
    bool dataSafe{false};
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe);

    switch (state) {
        case SMUX::ENABLE:
        enableREG |= 0x10; 
        break;
        
        case SMUX::DISABLE:
        enableREG &= 0b11101111;
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::ENABLE, enableREG);
    } else {
        return false;
    }
}

bool AS7341basic::enableWait(WAIT state) {
    bool dataSafe{false};
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe) | 0x08;

    switch (state) {
        case WAIT::ENABLE:
        enableREG |= 0x08;
        break;

        case WAIT::DISABLE:
        enableREG &= 0b11110111;
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::ENABLE, enableREG);
    } else {
        return false;
    }
}

bool AS7341basic::enableSpectrum(SPECTRUM state) {
    bool dataSafe{false};
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe) | 0x02;

    switch (state) {
        case SPECTRUM::ENABLE:
        enableREG |= 0x02;
        break;

        case SPECTRUM::DISABLE:
        enableREG &= 0b11111101;
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::ENABLE, enableREG);
    } else {
        return false;
    }
}

}