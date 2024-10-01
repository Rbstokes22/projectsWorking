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
    if (mAdriving < 4) mAdriving = 4;
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
// conditions. Default set to 9.
bool AS7341basic::setAGAIN(AGAIN val) {
    bool dataSafe{false}; // unused. Reserve bits are set to 0 anyway.
    uint8_t gainBit = static_cast<uint8_t>(val);

    // Or's the preserved register val with the gain data.
    uint8_t regVal = this->readRegister(REG::AGAIN, dataSafe) | gainBit;
    return this->validateWrite(REG::AGAIN, regVal);
}

}