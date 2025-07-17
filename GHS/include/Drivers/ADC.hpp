#ifndef ADC_DRVR_HPP
#define ADC_DRVR_HPP

#include "UI/MsgLogHandler.hpp"
#include "I2C/I2C.hpp"
#include "Config/config.hpp"

// Date sheet
// https://download.mikroe.com/documents/datasheets/ADS1115%20Datasheet.pdf

// ATTENTION: Addressing per the datasheet involves 4 different pins, one of 
// which is bridged to the addr pin. The dev addr per the briged pin is:
// GND 0b1001000 | VDD 0b1001001 | SDA 0b1001010 | SCL 0b1001011 

// WARNING: Single shot mode is the default and will be the only mode used
// here. This device has a single ADC, which requires multiplexer config
// before each reading, to read the correct pin. SINGLE_SHOT mode will remain
// the default.

namespace ADC_DRVR {

#define ADC_LOG_METHOD Messaging::Method::SRL_LOG // Default logging method.
#define ADC_TAG "(ADC)"
#define ADC_I2C_TIMEOUT 1000 // time in millis the i2c will timeout.
#define ADC_BAD_VAL -1 // Indicated bad value. Expect only positve values.
#define ADC_CONV_WAIT_MS 20 // Delay to wait for conversion.

// All enum classes correspond with the datasheet register val
enum class REG : uint8_t {
    CONVERSION, CONFIG, LO_THRESH, HI_THRESH
};

// Full Scale Range (FSR) is a programmable gain amplifier. Corresponds with
// the voltage difference. Choose the value higher than the input power
// such as 3.3V will use 4.096V. Default value is 2.048. Per datasheet 0.256 V
// corresond with 0b101 to 0b111. These voltages are +-. 
enum class FSR : uint8_t {
    V6_144, V4_096, V2_048, V1_024, V0_512, V0_256,
};

// Sample rate in Samples Per Second (SPS). Default value is 0b100 or 128 SPS.
enum class DATA_RATE : uint8_t {
    SPS8, SPS16, SPS32, SPS64, SPS128, SPS250, SPS475, SPS860
};

// Configuration. Adjust the gain and sampling rate as required.
struct CONF {
    float inputVoltage; // Should be 3.3 for esp-32.
    FSR gain; // Voltage gain amplifier.
    DATA_RATE sps;
};

class ADC {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    bool isInit; // Used to prevent double initiation of i2c resources.
    CONF pkt; // Configuration packet.
    i2c_master_dev_handle_t i2cHandle; // I2C handle to register device.
    bool config(uint8_t pinNum, bool refreshConf);
    int16_t getVal();
    bool isConverting(); // Check conversion status.
    void sendErr(const char* msg, 
            Messaging::Levels lvl = Messaging::Levels::ERROR);

    public:
    ADC();
    void init(uint8_t i2cAddr, CONF &pkt);
    void read(int16_t &readVar, uint8_t pin, bool refreshConf = false);
    CONF* getConf();
};

}

#endif // ADC_DRVR_HPP