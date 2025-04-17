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

// Requites state and verbose bool. Returns true of false if valid.
bool AS7341basic::power(PWR state, bool verbose) {
    return this->validateWrite(REG::ENABLE, static_cast<uint8_t>(state), 
        verbose);
}

// Requires value and verbose bool. Handles time between measurements.
// Returns true of false.
bool AS7341basic::configWTIME(uint8_t value, bool verbose) {
    return this->validateWrite(REG::WTIME, value, verbose);
}

// Requires SMUX_CONF::READ or SMUX_CONF::WRITE, and verbose bool. Writes
// the configuration to register. Returns true or false upon success.
bool AS7341basic::configSMUX(SMUX_CONF config, bool verbose) {
    static bool SMUX_INIT{false}; // Sent only once.
    bool dataSafe{false};
    uint8_t total{0}; // Total count of valid writes.
    uint8_t totalExp{1}; // Total Expected.

    // Reads the current register. Sets the init byte by clearing bits
    // 3 and 4. Sets conf byte by moving config to bits 3 and 4.
    uint8_t smux_conf = this->readRegister(REG::SMUX_CONFIG, dataSafe);
    uint8_t init = smux_conf & 0b11100111; 
    uint8_t conf = smux_conf | (static_cast<uint8_t>(config) << 3); 

    // Happens only once during operation. If not init, increments 
    // total expected. 
    if (!SMUX_INIT && dataSafe) {
        totalExp++;
        total += this->validateWrite(REG::SMUX_CONFIG, init, verbose);
        SMUX_INIT = true;
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Short delay between sends.

    if (dataSafe) total += this->validateWrite(REG::SMUX_CONFIG, conf, verbose);

    return (total == totalExp);
}

// Requires the LED_CONF:: ENABLE or DISABLE, and verbose bool. 
// Configures the LED. Returns true or false upon success.
bool AS7341basic::enableLED(LED_CONF state, bool verbose) {
    bool dataSafe{false};

    // Reads register to preserve data.
    uint8_t configREG = this->readRegister(REG::CONFIG, dataSafe);

    switch (state) {
        case LED_CONF::ENABLE:
        configREG |= 0x08; // Sets bit 3 to 1.
        break;

        case LED_CONF::DISABLE:
        configREG &= 0b11110111; // Sets bit 3 to 0.
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::CONFIG, configREG, verbose);
    } else {
        return false;
    }
}

// Requires SMUX::ENABLE or DISABLE, and verbose bool. Returns
// true or false upon success.
bool AS7341basic::enableSMUX(SMUX state, bool verbose) {
    bool dataSafe{false};

    // Reads register to preserve data.
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe);

    switch (state) {
        case SMUX::ENABLE:
        enableREG |= 0x10; // sets bit 4 to 1.
        break;
        
        case SMUX::DISABLE:
        enableREG &= 0b11101111; // clears bit 4.
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::ENABLE, enableREG, verbose);
    } else {
        return false;
    }
}

// Requires WAIT::ENABLE or DISABLE and verbose bool. Returns true
// or false upon success.
bool AS7341basic::enableWait(WAIT state, bool verbose) {
    bool dataSafe{false};

    // Reads register to preserve data.
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe) | 0x08;

    switch (state) {
        case WAIT::ENABLE:
        enableREG |= 0x08; // Sets bit 3 to 1.
        break;

        case WAIT::DISABLE:
        enableREG &= 0b11110111; // Clears bit 3.
        break;
    }

    if (dataSafe) {
        return this->validateWrite(REG::ENABLE, enableREG, verbose);
    } else {
        return false;
    }
}

// Requires SPECTRUM::ENABLE or DISABLE, and verbose bool. Returns
// true or false upon success.
bool AS7341basic::enableSpectrum(SPECTRUM state, bool verbose) {
    bool dataSafe{false};

    // Reads register to preserve data.
    uint8_t enableREG = this->readRegister(REG::ENABLE, dataSafe);

    switch (state) {
        case SPECTRUM::ENABLE:
        enableREG |= 0x02; // Sets bit 1 to 1.
        break;

        case SPECTRUM::DISABLE:
        enableREG &= 0b11111101; // Clears bit 1.
        break;
    }

    if (dataSafe) {
        bool write = this->validateWrite(REG::ENABLE, enableREG, verbose);

        // If bit 1 is set, shows enabled, If not, shows disabled.
        if (write) this->specEn = (enableREG >> 1) & 0b1; 

        return write;

    } else {
        snprintf(this->log, sizeof(this->log), "%s spec enable err", this->tag);
        this->sendErr(this->log);
        return false;
    }
}

}