#ifndef AS7341_LIBRARY_HPP
#define AS7341_LIBRARY_HPP

#include "stdint.h"
#include "I2C/I2C.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"

// Datasheet
// https://cdn.sparkfun.com/assets/0/8/e/2/3/AS7341_DS000504_3-00.pdf

namespace AS7341_DRVR {

#define AS7341_TIMEOUT 500 // 500 millis default timeout
#define AS7341_MIN 0 // 0 counts
#define AS7341_MAX 65535 // full 16 bit int
#define AS7341_WAIT 1000 // milliseconds timeout.
#define AS7341_MAX_LOGS 10 // Prevents intermittent errors from polluting log.

// All used register addresses used in the scope of this class.
enum class REG : uint8_t {
    ENABLE = 0x80, CONFIG = 0x70, REG_BANK = 0xA9,
    LED = 0x74, REG_STAT = 0x71, SPEC_STAT = 0xA3,
    SMUX_CONFIG = 0xAF,
    ATIME = 0x81, WTIME = 0x83, AGAIN = 0xAA,
    ASTEP_LWR = 0xCA, ASTEP_UPR = 0xCB,
    CH0_LWR = 0x95, CH0_UPR = 0x96, 
    CH1_LWR = 0x97, CH1_UPR = 0x98,
    CH2_LWR = 0x99, CH2_UPR = 0x9A,
    CH3_LWR = 0x9B, CH3_UPR = 0x9C,
    CH4_LWR = 0x9D, CH4_UPR = 0x9E,
    CH5_LWR = 0x9F, CH5_UPR = 0xA0
}; 

// Defines lower and upper addr for channels
extern REG CH_REG_MAP[6][2];

// Channel map to omit the need to explicitly call lower and
// upper address bytes. Works with the CH_REG_MAP.
enum class CHANNEL : uint8_t {
    CH0, CH1, CH2, CH3, CH4, CH5
};

// POWER ON and OFF
enum class PWR : uint8_t {OFF, ON};

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
        uint16_t ASTEP = AS7341_ASTEP, 
        uint8_t ATIME = AS7341_ATIME, 
        uint8_t WTIME = AS7341_WTIME
        );
};

// Each color defined by the color map of the datasheet.
struct COLOR {
    uint16_t F1_415nm_Violet;
    uint16_t F2_445nm_Indigo;
    uint16_t F3_480nm_Blue;
    uint16_t F4_515nm_Cyan;
    uint16_t F5_555nm_Green;
    uint16_t F6_590nm_Yellow;
    uint16_t F7_630nm_Orange;
    uint16_t F8_680nm_Red;
    uint16_t Clear;
    uint16_t NIR;
};

class AS7341basic {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    i2c_master_dev_handle_t i2cHandle; // handle for i2c init.
    CONFIG &conf; // Configuration
    bool isInit; // Is initialized.
    bool setBank(REG reg);
    bool writeRegister(REG reg, uint8_t val);
    uint8_t readRegister(REG reg, bool &dataSafe);
    bool validateWrite(REG reg, uint8_t dataOut, bool verbose = true);
    bool power(PWR state, bool verbose = true);
    bool configATIME(uint8_t value, bool verbose = true);
    bool configASTEP(uint16_t value, bool verbose = true);
    bool configWTIME(uint8_t value, bool verbose = true);
    bool configSMUX(SMUX_CONF config, bool verbose = true);
    bool enableLED(LED_CONF state, bool verbose = true);
    bool enableSMUX(SMUX state, bool verbose = true);
    bool enableWait(WAIT state, bool verbose = true);
    bool enableSpectrum(SPECTRUM state, bool verbose = true);
    bool delayIsReady(uint32_t timeout_ms);
    bool isReady();
    void setSMUXLowChannels(bool f1_f4);
    void setup_F1F4_Clear_NIR();
    void setup_F5F8_Clear_NIR();
    void sendErr(const char* msg, bool isLog = false, bool bypassLogMax = false,
        Messaging::Levels lvl = Messaging::Levels::ERROR);

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
    uint16_t readChannel(CHANNEL chnl, bool &dataSafe, bool delayEn = false);
    bool readAll(COLOR &color);
};

}

#endif // AS7341_HPP
