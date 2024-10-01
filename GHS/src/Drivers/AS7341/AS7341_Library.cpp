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

// Defines lower and upper addr for channels
REG CH_REG_MAP[6][2] = {
    {REG::CH0_LWR, REG::CH0_UPR},
    {REG::CH1_LWR, REG::CH1_UPR},
    {REG::CH2_LWR, REG::CH2_UPR},
    {REG::CH3_LWR, REG::CH3_UPR},
    {REG::CH4_LWR, REG::CH4_UPR},
    {REG::CH5_LWR, REG::CH5_UPR}
};

CONFIG::CONFIG(uint16_t ASTEP, uint8_t ATIME, uint8_t WTIME) : 

    ASTEP(ASTEP), ATIME(ATIME), WTIME(WTIME) {
        if (this->ASTEP > 65534) this->ASTEP = 65534;
    }

bool AS7341basic::delay(uint32_t timeout_ms) {
    if (timeout_ms == 0) { // Wait forever
        while(!this->isReady()) {
            ets_delay_us(1000); // delay 1 milli
        }
        return true;
    }

    if (timeout_ms > 0) {
        uint32_t elapsed{0};

        while(!this->isReady() && (elapsed < timeout_ms)) {
            ets_delay_us(1000);
            elapsed++;
            if (elapsed >= timeout_ms) return false;
        }
        return true;
    }

    return false;
}

bool AS7341basic::isReady() {
    bool dataSafe{false};
    uint8_t ready = this->readRegister(REG::SPEC_STAT, dataSafe) >> 6;
    if (dataSafe && (ready & 0b00000001)) {
        return true;
    } else {
        return false;
    }
}

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
    I2C::I2C_RET ret = I2C::i2c_master_init(I2C_DEF_FRQ);

    if (ret == I2C::I2C_RET::INIT_FAIL) {
        return false;
    } 

    // Compares at the end, expected validations vs actual validations.
    // If equal, sets isInit to true;
    uint8_t expVal{10};
    uint8_t actualVal{0};

    // Configure the i2c handle by adding device to I2C.
    i2c_device_config_t devCon = I2C::configDev(address);
    this->i2cHandle = I2C::addDev(devCon);

    actualVal += this->power(PWR::OFF); // Clear register before config
    actualVal += this->power(PWR::ON);
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


uint16_t AS7341basic::readChannel(CHANNEL chnl, bool &dataSafe) {
    REG CH_LWR = CH_REG_MAP[static_cast<uint8_t>(chnl)][0];
    REG CH_UPR = CH_REG_MAP[static_cast<uint8_t>(chnl)][1];

    bool lwrSafe{false}, uprSafe{false};
    bool ready = this->delay(1000);

    if (ready) {
        uint8_t ch_lwr = this->readRegister(CH_LWR, lwrSafe);
        uint8_t ch_upr = this->readRegister(CH_UPR, uprSafe);
        
        if (lwrSafe && uprSafe) {
            dataSafe = true;
            return (ch_upr << 8) | ch_lwr;

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

bool AS7341basic::readAll(COLOR &color) {
    bool safe[6] = {false, false, false, false, false, false};
    uint8_t safeExp = 10;
    uint8_t safeAct = 0;

    // The datasheet does not contain anything about mapping the F-channels to the
    // ADC. Referenced and did a little bit of reverse enginnering with the 
    // Adafruit_AS7341.cpp to configure everything below.

    this->setSMUXLowChannels(true);
    this->enableSpectrum(SPECTRUM::ENABLE, false);
    this->delay(0);

    color.F1_415nm_Violet = this->readChannel(CHANNEL::CH0, safe[0]);
    color.F2_445nm_Indigo = this->readChannel(CHANNEL::CH1, safe[1]);
    color.F3_480nm_Blue = this->readChannel(CHANNEL::CH2, safe[2]);
    color.F4_515nm_Cyan = this->readChannel(CHANNEL::CH3, safe[3]);

    for (int i = 0; i < sizeof(safe); i++) {
        safeAct += safe[i];
        safe[i] = false; // reset
    }

    this->setSMUXLowChannels(false);
    this->enableSpectrum(SPECTRUM::ENABLE, false);
    this->delay(0);

    color.F5_555nm_Green = this->readChannel(CHANNEL::CH0, safe[0]);
    color.F6_590nm_Yellow = this->readChannel(CHANNEL::CH1, safe[1]);
    color.F7_630nm_Orange = this->readChannel(CHANNEL::CH2, safe[2]);
    color.F8_680nm_Red = this->readChannel(CHANNEL::CH3, safe[3]);
    color.Clear = this->readChannel(CHANNEL::CH4, safe[4]);
    color.NIR = this->readChannel(CHANNEL::CH5, safe[5]);

    for (int i = 0; i < sizeof(safe); i++) {
        safeAct += safe[i];
    }

    return (safeAct == safeExp);
}



}
