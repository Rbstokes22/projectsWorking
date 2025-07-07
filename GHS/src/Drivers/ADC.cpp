#include "Drivers/ADC.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "I2C/I2C.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace ADC_DRVR {

// Requires the pin number to read, and if the configuration requires refresh,
// typically upon init, or if updates are required. Computes 16-bit config
// to send to config register to begin taking sample. Continuous mode is
// disabled for this application. Once config is called, delay briefly and 
// read conversion register.
void ADC::config(uint8_t pinNum, bool refresh) {

    // ATTENTION: Bit 15 will default to 0 which has no effect.

    static uint16_t conf = 0; // Init to zero, will use bitwise to populate.

    if (refresh) { // Refresh all configuration. Usually on init.
        conf = 0; // Sets single shot mode.
        conf |= (((pinNum & 0x3) + 4) << 12); // Sets MUX
        conf |= (static_cast<uint8_t>(this->pkt.gain) << 9); // Sets FSR
        conf |= (0b1 << 8); // Sets dev operating mode to single shot. (def)
        conf |= static_cast<uint8_t>(this->pkt.sps) << 5; // Sets SPS

        // OMIT bits 0 - 4. Leave default value of 0 and 0 leaving the 
        // comp mode in traditional comparator and polarity in active low.
        // WIll not be using any alert/ready ISR with the device.

    } else { // Refresh multiplexor only. Preserves bits. 

        // We will be using single shot mode and measuring against ground 
        // reference. Will use bits >= 0b100 per datasheet Table 8.
        conf &= ~(0x7 << 12); // Clear bits 12 - 14.
        conf |= (((pinNum & 0x3) + 4) << 12); // Populate bits 12-14.
    }

    uint8_t writeBuf[3] = { 
        static_cast<uint8_t>(REG::CONFIG), // REG
        ((conf >> 8) & 0xFF), // MSB
        conf & 0xFF // LSB
    };

    esp_err_t err = i2c_master_transmit(this->i2cHandle, writeBuf, 
        sizeof(writeBuf), ADC_I2C_TIMEOUT);

    if (err != ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s writing config", this->tag);
        this->sendErr(this->log);
    }
}

int16_t ADC::getVal(uint8_t pinNum) {

    // Scale actual reading to 16-bit count to show max and min values due to
    // how the FSR works.
    int16_t val = ADC_BAD_VAL;
    float FSR = -1.0f; // Prevents a div by 0, just in case.

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
    float multiplier = this->pkt.inputVoltage / FSR;

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

// Requires message and messaging level, which is default to ERROR.
void ADC::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, ADC_LOG_METHOD);
}

ADC::ADC() : tag(ADC_TAG), log(0), isInit(false) {

    memset(&this->pkt, 0, sizeof(this->pkt)); // Clear packet.

    snprintf(this->log, sizeof(this->log), "%s Ob Created", ADC_TAG);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

void ADC::init(uint8_t i2cAddr, CONF &pkt) {
    if (this->isInit) return; // Block if already init.

    // Init I2C and add device to the service
    Serial::I2C* i2c = Serial::I2C::get();
    i2c_device_config_t devCon = i2c->configDev(i2cAddr);
    this->i2cHandle = i2c->addDev(devCon);
    
    this->pkt = pkt; // Set the class object pkt upon init.
    this->isInit = true;
}

// Requires pin number, default to 4 which indicates all. If value exceeds 4,
// 4 will be the max number. The read packet must be a 4 int array, which will
// populate the value to the corresponding pin reading (A0 = idx 0). A value
// of -1 indicates error. Refresh is default to false, when set to true, will
// reset the current configuration to 0, to reconfigure. When false, will only
// reconfigure the multiplexer.
void ADC::read(uint8_t pin, int* readPkt, bool refresh) {

    if (!this->isInit) {
        snprintf(this->log, sizeof(this->log), "%s Not init", this->tag);
        this->sendErr(this->log);
        return; // Block
    }

    if (readPkt == nullptr) { 
        snprintf(this->log, sizeof(this->log), "%s nullptr passed", this->tag);
        this->sendErr(this->log);
        return; // Block
    }

    if (pin > 4) pin = 4; // Ensure max capacity is 4 pins for the ADS1115.

    for (uint8_t i = 0; i < pin; i++) {
        this->config(i, refresh); // Writes to config register.
        vTaskDelay(pdMS_TO_TICKS(20)); // Allow computation and conversion.
        readPkt[i] = this->getVal(i); // Set idx to computed val.
    }
}

// Returns packet for updates if required. Packet will be used for each read.
CONF* ADC::getConf() {return &this->pkt;}

}