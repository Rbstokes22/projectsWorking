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

// Requires LED state on or off, and milliamp brightness
// between 4mA and 258mA. Illuminates LED to set value.
bool AS7341basic::setLEDCurrent(LED state, uint16_t mAdriving) {
    if (mAdriving < 4) mAdriving = 4;
    if (mAdriving > 258) mAdriving = 258;

    // Results in a value between 0 and 127.
    uint8_t drive = 0b10000000 | ((mAdriving - 4) >> 1);

    switch (state) {
        case LED::ON:
        return this->validateWrite(REG::LED, drive);

        case LED::OFF:
        return this->validateWrite(REG::LED, 0b00000000);

        default:
        return false;
    }
}

// Requires AGAIN value, returns bool if successfully written.
// Allows the sensor gain to be dynamically set based upon light
// conditions. Default set to 9.
bool AS7341basic::setAGAIN(AGAIN val) {
    return this->validateWrite(REG::AGAIN, static_cast<uint8_t>(val));
}

}