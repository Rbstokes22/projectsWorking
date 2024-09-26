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

CONFIG::CONFIG(uint16_t ASTEP, uint8_t ATIME, uint8_t WTIME) : 

    ASTEP(ASTEP), ATIME(ATIME), WTIME(WTIME) {
        if (this->ASTEP > 65534) this->ASTEP = 65534;
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
    uint8_t expVal{9};
    uint8_t actualVal{0};

    // Configure the i2c handle by adding device to I2C.
    i2c_device_config_t devCon = I2C::configDev(address);
    this->i2cHandle = I2C::addDev(devCon);

    actualVal += this->powerOn();
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

// If timeoutMicros = -1, wait indefinitely.
uint16_t AS7341basic::readChannel(
    REG channel, 
    int timeoutMicros, 
    bool &dataSafe
    ) {

    int64_t start = esp_timer_get_time();
    int64_t _timeout = start + timeoutMicros;

    bool readySafe{false}, lwrSafe{false}, uprSafe{false};

    uint16_t CHaddr = static_cast<uint16_t>(channel);
    REG CH_LWR = static_cast<REG>(CHaddr & 0xFF);
    REG CH_UPR = static_cast<REG>(CHaddr >> 8);

    while(true) {
        // Check the LSB to see if data is ready.
        uint8_t dataReady = this->readRegister(REG::REG_STAT, readySafe) & 0x01;

        if (dataReady == 1 && readySafe) {
            uint8_t ch_lwr = this->readRegister(CH_LWR, lwrSafe);
            uint8_t ch_upr = this->readRegister(CH_UPR, uprSafe);
         
            if (lwrSafe && uprSafe) {
                dataSafe = true;
                return (ch_upr << 8) | ch_lwr;

            } else {
                dataSafe = false;
                return 0;
            }
        } 

        // Times out when conditions above are not met.
        if (timeoutMicros != -1) {
            if (esp_timer_get_time() > _timeout) {
                printf("AS7341 Timed Out\n");
                dataSafe = false;
                return 0;
            }
        }
     
        ets_delay_us(10);
    }
}

}
