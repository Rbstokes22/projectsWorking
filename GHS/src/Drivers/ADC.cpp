#include "Drivers/ADC.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "I2C/I2C.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Config/config.hpp"

namespace ADC_DRVR {

// Requires pin number, and bool to refresh config, which is usually called
// initially or if changes are desired. Setting to false will
// preserve bits and only take the reading. Returns true if successful, and 
// false if not. Function name is a misnomer, but named this due to req of 
// writing to config register to begin reading.
bool ADC::config(uint8_t pinNum, bool refreshConf) {

    // ATTENTION: Bit 15 will default to 0 which has no effect.

    // First ensure that a conversion is not occuring.
    uint8_t count = 0;
    while(this->isConverting() && count <= ADC_CONV_WAIT_MS) {
        count++;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (count >= ADC_CONV_WAIT_MS) { // Timed out due to counting.
        snprintf(this->log, sizeof(this->log), "%s config T/O", this->tag);
        this->sendErr(this->log);
        return false; // Block.
    }

    static uint16_t conf = 0; // Init to zero, will use bitwise to populate.

    if (refreshConf) { // Refresh all configuration. Usually on init.
        conf = 0x8000; // Sets single shot mode.
        conf |= (((pinNum & 0x3) + 4) << 12); // Sets MUX
        conf |= (static_cast<uint8_t>(this->pkt.gain) << 9); // Sets FSR
        conf |= (0b1 << 8); // Sets dev operating mode to single shot. (def)
        conf |= static_cast<uint8_t>(this->pkt.sps) << 5; // Sets SPS

        // OMIT bits 0 - 4. Leave default value of 0 and 0 leaving the 
        // comparator mode in traditional and polarity in active low.
        // Will not be using any alert/ready ISR with the device.

    } else { // Refresh multiplexor only. Preserves bits. 

        // We will be using single shot mode and measuring against ground 
        // reference. Will use bits >= 0b100 per datasheet Table 8.
        conf &= ~(0x7 << 12); // Clear bits 12 - 14.
        conf |= (((pinNum & 0x3) + 4) << 12); // Populate bits 12-14.
    }

    uint8_t writeBuf[3] = { // Write buffer, requires REG, and 16-bit command.
        static_cast<uint8_t>(REG::CONFIG), // REG
        static_cast<uint8_t>(((conf >> 8) & 0xFF)), // MSB
        static_cast<uint8_t>(conf & 0xFF) // LSB
    };

    esp_err_t err = i2c_master_transmit(this->i2cHandle, writeBuf, 
        sizeof(writeBuf), ADC_I2C_TIMEOUT);

    if (err != ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s writing config", this->tag);
        this->sendErr(this->log);
        return false;
    }

    return true;

}

// Gets value from single conversion register. WARNING!!! Value must be 
// captured before taking another reading due to the single ADC and Conversion
// register on board. Returns the int16 value.
int16_t ADC::getVal() {

    // First ensure that a conversion is not occuring.
    uint8_t count = 0;
    while(this->isConverting() && count <= ADC_CONV_WAIT_MS) {
        count++;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (count > ADC_CONV_WAIT_MS) { // Timed out due to counting.
        snprintf(this->log, sizeof(this->log), "%s get val T/O", this->tag);
        this->sendErr(this->log);
        return ADC_BAD_VAL; 
    }

    // Scale actual reading to 16-bit count to show max and min values due to
    // how the FSR works.
    int16_t val = ADC_BAD_VAL; // Default to the bad val, unless changed.
    float FSR = 1.0f; // Prevents a div by 0, just in case.

    // Sets the FSR gain voltage value. Used only to normalize output.
    switch (static_cast<uint8_t>(this->pkt.gain)) {
        case 0: FSR = 6.144; break;
        case 1: FSR = 4.096; break;
        case 2: FSR = 2.048; break;
        case 3: FSR = 1.024; break;
        case 4: FSR = 0.512; break;
        case 5: FSR = 0.256; break;
    }

    // Multiplier is used to increase the max reading to a max value. For ex
    // a 3.3 v with an FSR of 4.096 will only read a max of 80.5% of the max
    // value of 32767, so we want to multiply our reading by the multiplier to
    // scale it up as if it were 4.096V input.
    float multiplier = FSR / this->pkt.inputVoltage;

    uint8_t writeBuf[1] = {static_cast<uint8_t>(REG::CONVERSION)};
    uint8_t readBuf[2] = {0, 0}; // Init to 0.
    
    // Request to read 16 bits from the conversion register.
    esp_err_t err = i2c_master_transmit_receive(
        this->i2cHandle, writeBuf, sizeof(writeBuf), readBuf, sizeof(readBuf),
        ADC_I2C_TIMEOUT
    );

    if (err == ESP_OK) { // Good read, populate data into int16_t
        val = (readBuf[0] << 8) | readBuf[1]; // Combine MSB and LSB

        float scaled = val * multiplier; // Check scaled range

        // Check for bounds.
        if (scaled > INT16_MAX) scaled = INT16_MAX;
        else if (scaled < INT16_MIN) scaled = INT16_MIN;

        val = static_cast<int16_t>(scaled);
    }

    return val; // -1 indicates a bad read.
}

// Requires no params. Reads the configuration register of the device to check
// the 15th bit. Returns true if device is currently converting, and false if
// not.
bool ADC::isConverting() {

    uint8_t writeBuf[1] = {static_cast<uint8_t>(REG::CONFIG)};
    uint8_t readBuf[2] = {0, 0};

    esp_err_t err = i2c_master_transmit_receive(
        this->i2cHandle, writeBuf, sizeof(writeBuf), readBuf, sizeof(readBuf),
        ADC_I2C_TIMEOUT
    );

    if (err != ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s error reading conf reg", 
            this->tag);

        this->sendErr(this->log);
        return true; // Returns true since this will be a blocker.
    }

    // Per datasheet, a 0 indicates the device is currently performing a conv,
    // and 1 is the opposite. Read byte and invert logic to meet req. This would
    // not use the ! operator if the func was called doneConverting().
    return !((readBuf[0] >> 7) & 0b1); 
}

// Requires message and messaging level, which is default to ERROR.
void ADC::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, ADC_LOG_METHOD);
}

ADC::ADC() : tag(ADC_TAG), log(0), isInit(false) {

    memset(&this->pkt, 0, sizeof(this->pkt)); // Clear packet.

    snprintf(this->log, sizeof(this->log), "%s Ob Created", ADC_TAG);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Requires the i2c address and confirmation packet. Initializes the ADC and 
// configures the device. Must occur before reading.
void ADC::init(uint8_t i2cAddr, CONF &pkt) {
    if (this->isInit) return; // Block if already init.

    // Init I2C and add device to the service
    Serial::I2C* i2c = Serial::I2C::get();
    i2c_device_config_t devCon = i2c->configDev(i2cAddr);
    this->i2cHandle = i2c->addDev(devCon);
    
    this->pkt = pkt; // Set the class object pkt upon init.
    this->isInit = true;
    this->config(0, true); // Configures the device upon init.
}

// Requires the reference to the int16 variable that the reading will populate,
// the pin number, and if the configuration need to be refreshed, which is 
// default to false. Reads the ADC pin. A return value of ADC_BAD_VAL will mean
// the value is bad. This is designed for single point measurement and not
// differential.
void ADC::read(int16_t &readVar, uint8_t pin, bool refreshConf) {

    if (!this->isInit) { // Ensures the device has been init before reading.
        snprintf(this->log, sizeof(this->log), "%s Not init", this->tag);
        this->sendErr(this->log);
        return; // Block
    }

    if (pin > 3) {
        snprintf(this->log, sizeof(this->log), "%s pin must be between 0 - 3", 
            this->tag);

        this->sendErr(this->log);
        return; // Block
    }

    if (this->config(pin, refreshConf)) {
        readVar = this->getVal();
    }
}

// Returns packet for updates if required. Packet will be used for each read.
CONF* ADC::getConf() {return &this->pkt;}

}