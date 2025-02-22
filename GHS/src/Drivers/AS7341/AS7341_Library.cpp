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

// Defines lower and upper addr for channels to avoid specifically calling the 
// lower and upper register address for each call.
REG CH_REG_MAP[6][2] = {
    {REG::CH0_LWR, REG::CH0_UPR},
    {REG::CH1_LWR, REG::CH1_UPR},
    {REG::CH2_LWR, REG::CH2_UPR},
    {REG::CH3_LWR, REG::CH3_UPR},
    {REG::CH4_LWR, REG::CH4_UPR},
    {REG::CH5_LWR, REG::CH5_UPR}
};

// Used to configure AS7341 upon init. ASTEP and ATIME are both full value 
// uint8_t integers. ASTEP is between 0 & 65534. Requires ASTEP, ATIME, 
// and WTIME values.
CONFIG::CONFIG(uint16_t ASTEP, uint8_t ATIME, uint8_t WTIME) : 

    ASTEP(ASTEP), ATIME(ATIME), WTIME(WTIME) {
        // Used if set to 65535.
        if (this->ASTEP > 65534) this->ASTEP = 65534;
    }

// Requires timeout in millis. Executes a loop that will be satisfied when the 
// spectral register signals that the data is ready for processing. If 0 is 
// passed, will wait indefinitely for register. If a value is passed, loop 
// executes with a 1 millisecond delay until ready or timeout is reached. 
// Returns true if ready, and false if timeout reached.
bool AS7341basic::delayIsReady(uint32_t timeout_ms) {
    if (timeout_ms == 0) { // Wait forever
        while(!this->isReady()) {
            ets_delay_us(1000); // delay 1 milli
        }
        return true;
    }

    if (timeout_ms > 0) {
        uint32_t elapsed{0};

        while(!this->isReady() && (elapsed < timeout_ms)) {
            ets_delay_us(1000); // Delay 1 milli
            elapsed++;
            if (elapsed >= timeout_ms) return false;
        }
        return true;
    }

    return false;
}

// Reads register that signals if the spectral measurment is ready.
// Returns true of false based upon ready value.
bool AS7341basic::isReady() {
    bool dataSafe{false};

    // Reads the 6th bit of the spectral status register. If that value
    // is 1, this indicates that the reading is ready.
    uint8_t ready = this->readRegister(REG::SPEC_STAT, dataSafe) >> 6;
    if (dataSafe && (ready & 0b00000001)) {
        return true;
    } else {
        return false;
    }
}

// Requires boolean if f1 to f4. Disables the spectrum and configs the
// SMUX to write followed by setting up the appropriate registers.
void AS7341basic::setSMUXLowChannels(bool f1_f4) {
    this->enableSpectrum(SPECTRUM::DISABLE, false);
    this->configSMUX(SMUX_CONF::WRITE, false);

    if (f1_f4) {
        this->setup_F1F4_Clear_NIR();
    } else {
        this->setup_F5F8_Clear_NIR();
    }

    this->enableSMUX(SMUX::ENABLE, false);
}

// Sets up channels F1 to F4, Clear, and NIR by writing register 1 - 19.
// This was taken from the Adafruit_AS7341 library, since this mapping was
// not available in the datasheet. This works in comparison with the 
// Adafruit upload.
void AS7341basic::setup_F1F4_Clear_NIR() {
    // SMUX Config for F1,F2,F3,F4,NIR,Clear
    this->writeRegister(static_cast<REG>(0x00),0x30); // F3 left set to ADC2
    this->writeRegister(static_cast<REG>(0x01), 0x01); // F1 left set to ADC0
    this->writeRegister(static_cast<REG>(0x02), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x03), 0x00); // F8 left disabled
    this->writeRegister(static_cast<REG>(0x04), 0x00); // F6 left disabled
    this->writeRegister(
        static_cast<REG>(0x05),
        0x42); // F4 left connected to ADC3/f2 left connected to ADC1
    this->writeRegister(static_cast<REG>(0x06), 0x00); // F5 left disbled
    this->writeRegister(static_cast<REG>(0x07), 0x00); // F7 left disbled
    this->writeRegister(static_cast<REG>(0x08), 0x50); // CLEAR connected to ADC4
    this->writeRegister(static_cast<REG>(0x09), 0x00); // F5 right disabled
    this->writeRegister(static_cast<REG>(0x0A), 0x00); // F7 right disabled
    this->writeRegister(static_cast<REG>(0x0B), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x0C), 0x20); // F2 right connected to ADC1
    this->writeRegister(static_cast<REG>(0x0D), 0x04); // F4 right connected to ADC3
    this->writeRegister(static_cast<REG>(0x0E), 0x00); // F6/F8 right disabled
    this->writeRegister(static_cast<REG>(0x0F), 0x30); // F3 right connected to AD2
    this->writeRegister(static_cast<REG>(0x10), 0x01); // F1 right connected to AD0
    this->writeRegister(static_cast<REG>(0x11), 0x50); // CLEAR right connected to AD4
    this->writeRegister(static_cast<REG>(0x12), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x13), 0x06); // NIR connected to ADC5
}

// The same as above, but for channels F5 to F8.
void AS7341basic::setup_F5F8_Clear_NIR() {
    // SMUX Config for F5,F6,F7,F8,NIR,Clear
    this->writeRegister(static_cast<REG>(0x00),0x00); // F3 left disable
    this->writeRegister(static_cast<REG>(0x01), 0x00); // F1 left disable
    this->writeRegister(static_cast<REG>(0x02), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x03), 0x40); // F8 left connected to ADC3
    this->writeRegister(static_cast<REG>(0x04), 0x02); // F6 left connected to ADC1
    this->writeRegister(static_cast<REG>(0x05), 0x00); // F4/F2 Disabled
    this->writeRegister(static_cast<REG>(0x06), 0x10); // F5 left connected to ADC0
    this->writeRegister(static_cast<REG>(0x07), 0x03); // F7 left connected to ADC2
    this->writeRegister(static_cast<REG>(0x08), 0x50); // CLEAR connected to ADC4
    this->writeRegister(static_cast<REG>(0x09), 0x10); // F5 right connected to ADC0
    this->writeRegister(static_cast<REG>(0x0A), 0x03); // F7 right connected to ADC2
    this->writeRegister(static_cast<REG>(0x0B), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x0C), 0x00); // F2 right disabled
    this->writeRegister(static_cast<REG>(0x0D), 0x00); // F4 right disabled
    this->writeRegister(
        static_cast<REG>(0x0E), 
        0x24); // F8 right connected to ADC2, F6 right connected to ADC1
    this->writeRegister(static_cast<REG>(0x0F), 0x00); // F3 right disabled
    this->writeRegister(static_cast<REG>(0x10), 0x00); // F1 right disabled
    this->writeRegister(static_cast<REG>(0x11), 0x50); // CLEAR right connected to AD4
    this->writeRegister(static_cast<REG>(0x12), 0x00); // Reserved or disabled
    this->writeRegister(static_cast<REG>(0x13), 0x06); // NIR connected to ADC5
}

AS7341basic::AS7341basic(CONFIG &conf) : conf(conf), isInit(false) {}

// Requires the address of the device. Initializes the I2C connection
// and writes to the device all configuration settings. Returns true 
// if successful or false if not.
bool AS7341basic::init(uint8_t address) {
    if (this->isInit) return true; // Prevents from being re-init.

    Serial::I2C* i2c = Serial::I2C::get();
    i2c_device_config_t devCon = i2c->configDev(address);
    this->i2cHandle = i2c->addDev(devCon);

    // Compares at the end, expected validations vs actual validations.
    // If equal, sets isInit to true;
    const uint8_t expVal{10};
    uint8_t actualVal{0};

    actualVal += this->power(PWR::OFF); // Clear register before config
    actualVal += this->power(PWR::ON); // Powers on device.
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay of > 200 micros req by datasheet.
    actualVal += this->configATIME(this->conf.ATIME);
    actualVal += this->configASTEP(this->conf.ASTEP);
    actualVal += this->configWTIME(this->conf.WTIME);

    // Write SMUX configuration from RAM to SMUX chain.
    actualVal += this->configSMUX(SMUX_CONF::WRITE);
    actualVal += this->enableLED(LED_CONF::ENABLE);
    actualVal += this->enableSMUX(SMUX::ENABLE);
    actualVal += this->enableWait(WAIT::ENABLE);
    actualVal += this->enableSpectrum(SPECTRUM::ENABLE);
    
    this->setLEDCurrent(LED::OFF); // Ensure LED is off
    this->isInit = (actualVal == expVal);
    return this->isInit;
}

// Returns if the devices has been properly initialized.
bool AS7341basic::isDevInit() {return this->isInit;}

// Requires the channel to be read, dataSafe bool, and delay Enable bool.
// the delayEn is set to false by default. If a channel read is performed
// outside of the readAll function, the delay should be enabled. Reads the
// lower and upper register addresses, and combines the two 8-bit values 
// into a 16-bit value for that channel. Returns the value along with the
// dataSafe bool.
uint16_t AS7341basic::readChannel(CHANNEL chnl, bool &dataSafe, bool delayEn) {
    REG CH_LWR = CH_REG_MAP[static_cast<uint8_t>(chnl)][0];
    REG CH_UPR = CH_REG_MAP[static_cast<uint8_t>(chnl)][1];

    bool lwrSafe{false}, uprSafe{false};

    // Designed for reading channel individually instead of all at once. If 
    // not-en, defaults to true. The delay is managed within the readAll
    // method which is why this is not-en and ready is set to true.
    bool ready = delayEn ? this->delayIsReady(AS7341_WAIT) : true;

    if (ready) {
        uint8_t ch_lwr = this->readRegister(CH_LWR, lwrSafe);
        uint8_t ch_upr = this->readRegister(CH_UPR, uprSafe);
        
        if (lwrSafe && uprSafe) {
            dataSafe = true;
            return (ch_upr << 8) | ch_lwr; // combines data into 16-bits.

        } else {
            dataSafe = false;
            return 0;
        }

    } else {
        dataSafe = false;
        printf("Timed Out\n");
        return 0;
    }
}

// Requires the COLOR struct with F1 - F8, clear, and NIR uint16_t data.
// Reads each channel into the struct. Returns true if the data is valid
// and false if invalid.
bool AS7341basic::readAll(COLOR &color) {
    bool safe[6] = {false, false, false, false, false, false}; // Ch0 - Ch5
    const uint8_t safeExp = 10; // 10 channel reads.
    uint8_t safeAct = 0;

    // The datasheet does not contain anything about mapping the F-channels to the
    // ADC. Referenced and did a little bit of reverse enginnering with the 
    // Adafruit_AS7341.cpp to configure everything below.

    this->setSMUXLowChannels(true); // sets channels F1 - F4.
    this->enableSpectrum(SPECTRUM::ENABLE, false); // enables spectrum
    this->delayIsReady(AS7341_WAIT); // Enable delay while waiting for ready. 

    // Reads F1 to F4. Will read Clear and NIR below.
    color.F1_415nm_Violet = this->readChannel(CHANNEL::CH0, safe[0]);
    color.F2_445nm_Indigo = this->readChannel(CHANNEL::CH1, safe[1]);
    color.F3_480nm_Blue = this->readChannel(CHANNEL::CH2, safe[2]);
    color.F4_515nm_Cyan = this->readChannel(CHANNEL::CH3, safe[3]);

    // Computes the qty of safe data values and resets the array.
    for (int i = 0; i < sizeof(safe); i++) {
        safeAct += safe[i];
        safe[i] = false; // reset
    }

    // Mirrors above but sets channels up for F5 - F8.
    this->setSMUXLowChannels(false);
    this->enableSpectrum(SPECTRUM::ENABLE, false);
    this->delayIsReady(AS7341_WAIT); // Enable delay while waiting for ready.

    color.F5_555nm_Green = this->readChannel(CHANNEL::CH0, safe[0]);
    color.F6_590nm_Yellow = this->readChannel(CHANNEL::CH1, safe[1]);
    color.F7_630nm_Orange = this->readChannel(CHANNEL::CH2, safe[2]);
    color.F8_680nm_Red = this->readChannel(CHANNEL::CH3, safe[3]);
    color.Clear = this->readChannel(CHANNEL::CH4, safe[4]);
    color.NIR = this->readChannel(CHANNEL::CH5, safe[5]);

    // Computes the qty of safe data values for a total.
    for (int i = 0; i < sizeof(safe); i++) {
        safeAct += safe[i];
    }

    // Returns true if the actual safe values equal the expected.
    return (safeAct == safeExp);
}

}
