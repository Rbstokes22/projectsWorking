#ifndef AS7341_LIBRARY_HPP
#define AS7341_LIBRARY_HPP

#include "stdint.h"
#include "I2C/I2C.hpp"

namespace AS7341_DRVR {

// All used register addresses.
enum class REG : uint16_t {
    ENABLE = 0x80, CONFIG = 0x70, REG_ACCESS = 0xA9,
    LED = 0x74, REG_STAT = 0x71,
    SMUX_CONFIG = 0xAF,
    ATIME = 0x81, WTIME = 0x83, AGAIN = 0xAA,
    ASTEP = 0xCB << 8 | 0xCA,
    CH0 = 0x96 << 8 | 0x95,
    CH1 = 0x98 << 8 | 0x97,
    CH2 = 0x9A << 8 | 0x99,
    CH3 = 0x9C << 8 | 0x9B,
    CH4 = 0x9E << 8 | 0x9D,
    CH5 = 0xA0 << 8 | 0x9F
}; 

// LED ON and OFF
enum class LED : uint8_t {OFF, ON};

// Analog Gain allows you to set the spectral sensitivity 
// dynamically. Default set at X256.
enum class AGAIN : uint8_t {
    X_HALF, X1, X2, X4, X8, X16, X32, X64, X128, X256, X512
};

// Enables or Disables the spectrum analysis
enum class SPECTRUM : uint8_t {ENABLE, DISABLE};

// Enables or Disables the wait time between two consecutive
// spectal measurements.
enum class WAIT : uint8_t {ENABLE, DISABLE};

// Enables or Disables SMUX.
enum class SMUX : uint8_t {ENABLE, DISABLE};

// Read SMUX config to RAM from SMUX chain. Write SMUX config
// from RAM to SMUX chain.
enum class SMUX_CONF : uint8_t {READ = 1, WRITE};

// Enables or Disables the LED.
enum class LED_CONF : uint8_t {ENABLE, DISABLE};

// ASTEP 0 - 65534. (Recommended 599)
// ATIME 0 - 255. (Recommended 29)
// WTIME 0 - 255. (Recommended 0)
// Integration time is (ATIME + 1) * (ASTEP + 1) * 2.78µs
struct CONFIG {
    uint16_t ASTEP;
    uint8_t ATIME;
    uint8_t WTIME;
    CONFIG(
        uint16_t ASTEP = 599, 
        uint8_t ATIME = 29, 
        uint8_t WTIME = 0
        );
};

class AS7341basic {
    private:
    i2c_master_dev_handle_t i2cHandle;
    CONFIG &conf;
    bool isInit;
    bool prepRegister(REG reg);
    void writeRegister(REG reg, uint8_t val);
    uint8_t readRegister(REG reg, bool &dataSafe);
    bool validateWrite(REG reg, uint8_t dataOut, bool verbose = true);
    bool powerOn();
    bool configATIME(uint8_t value);
    bool configASTEP(uint16_t value);
    bool configWTIME(uint8_t value);
    bool configSMUX(SMUX_CONF config);
    bool enableLED(LED_CONF state);
    bool enableSMUX(SMUX state);
    bool enableWait(WAIT state);
    bool enableSpectrum(SPECTRUM state);

    public:
    AS7341basic(CONFIG &conf);
    bool init(uint8_t address);
    bool isDevInit();
    bool setLEDCurrent(LED state, uint16_t mAdriving = 12);
    bool setAGAIN(AGAIN val);
    uint16_t getLEDCurrent(bool &dataSafe);
    uint8_t getAGAIN_RAW(bool &dataSafe);
    float getAGAIN(bool &dataSafe);
    uint16_t getASTEP_RAW(bool &dataSafe);
    float getASTEP(bool &dataSafe);
    uint8_t getATIME_RAW(bool &dataSafe);
    float getWTIME(bool &dataSafe);
    uint8_t getWTIME_RAW(bool &dataSafe);
    float getIntegrationTime(bool &dataSafe);
    uint8_t getSMUX(bool &dataSafe);
    bool getWaitEnabled(bool &dataSafe);
    bool getSpectrumEnabled(bool &dataSafe);

    uint16_t readChannel(
        REG channel, 
        int timeoutMicros, 
        bool &dataSafe
        );

};

}

#endif // AS7341_HPP
