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

// Requires LED::ON or OFF, and the milliamp setting between 4 mA
// and 258 mA. Any values outside of that range will be set to the
// closest range boundary (Ex: 290 will be set to 258). Returns
// true or false if written correctly.
bool AS7341basic::setLEDCurrent(LED state, uint16_t mAdriving) {
    if (mAdriving < 4) mAdriving = 4; // Datasheet requirements.
    if (mAdriving > 258) mAdriving = 258;

    // Range of mA is 4 to 258. bit 7 turns the LED on or off.
    // 4 is subtracted from the driving mA and the value is 
    // shifted by 1 to half it. Bits 0-6 are the driving mA
    // data bits.
    uint8_t drive = 0b10000000 | ((mAdriving - 4) >> 1);

    switch (state) {
        case LED::ON:
        return this->validateWrite(REG::LED, drive);

        case LED::OFF:
        // Ensures bit 7 is 0 to turn off LED.
        return this->validateWrite(REG::LED, 0b00000000);

        default:
        return false; // required to prevent wreturn error.
    }
}

// Requires AGAIN value, returns bool if successfully written.
// Allows the sensor gain to be dynamically set based upon light
// conditions. Default set to 9 or 256x. Does not need to disable spectral
// since this is dynamically set.
bool AS7341basic::setAGAIN(AGAIN val) {
    bool dataSafe{false}; // unused. Reserve bits are set to 0 default.
    uint8_t gain = static_cast<uint8_t>(val);

    // Or's the preserved register val with the gain data. Preserve only bits
    // 7:5.
    uint8_t regVal = this->readRegister(REG::AGAIN, dataSafe) & 0b11100000;
    regVal |= gain;
    return this->validateWrite(REG::AGAIN, regVal);
}

// Requires the value. Sets the integration time for 
// spectral measurments. Determins how long the sensor collects light before
// quantifying the measurement. Disables the spectrum prior to config, and
// re-enables prior to return. Returns true or false depending on success.
// The value 29 is recommended per the datasheet.
bool AS7341basic::setATIME(uint8_t value) {

    // Standard err message for ATIME
    snprintf(this->log, sizeof(this->log), "%s ATIME conf err", this->tag); 
    bool disabledHere = false; // Disabled spec by this function.
    bool success = false;
    
    if (specEn) { // If enabled, disable.
        
        if (!this->enableSpectrum(SPECTRUM::DISABLE, false)) { 
            this->sendErr(this->log);
            return false;
        }
        disabledHere = true; // Shows this function disabled spec.
    }

    success = this->validateWrite(REG::ATIME, value, true); // Write to reg.

    if (!success) this->sendErr(this->log);

    if (disabledHere) { // If disabled by this function, re-enable.
        if (!this->enableSpectrum(SPECTRUM::ENABLE, false)) { 
            this->sendErr(this->log);
            return false; // False because spectrum cant be restarted.
        }
    }

    return success; // True if all went well.
}

// Requires the value. Configures step time for SMUX 
// operations. Controls the timing of how long the sensor waits between 
// switching different photo diodes. Writes two 8-bit registers equating to
// a 16-bit value. Disables the spectrum prior to config, and re-enables before
// return. Returns true or false depending on success. The recommended value is 
// 599 per the datasheet.
bool AS7341basic::setASTEP(uint16_t value) {

    // Standard err message for ASTEP
    snprintf(this->log, sizeof(this->log), "%s ASTEP conf err", this->tag); 
    bool disabledHere = false; // Disabled spec by this function.
    uint8_t success = 0; // More than 1, cant use bool like ATIME.
    uint8_t expSucc = 2; // Expected successes.

    if (specEn) { // If enabled, disable.
        if (!this->enableSpectrum(SPECTRUM::DISABLE, false)) { 
            this->sendErr(this->log);
            return false;
        }
        disabledHere = true; // Shows this function disabled spec.
    }

    uint8_t ASTEPdata = 0x00 | (value & 0x00FF); // lower byte config
    if (value >= 65535) value = 65534; // 65535 is reserved per datasheet.

    success += this->validateWrite(REG::ASTEP_LWR, ASTEPdata, true); // LWR
    ASTEPdata = 0x00 | (this->conf.ASTEP >> 8); // upper byte config
    success += this->validateWrite(REG::ASTEP_UPR, ASTEPdata, true); // UPR

    if (disabledHere) { // If disabled by this function, re-enable.
        if (!this->enableSpectrum(SPECTRUM::ENABLE, false)) { 
            this->sendErr(this->log);
            return false; // False because spectrum cant be restarted.
        }
    }

    if (success != expSucc) this->sendErr(this->log); // Log if failed.
    
    return (success == expSucc); // If equal returns true;
}

}