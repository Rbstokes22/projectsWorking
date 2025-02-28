#include "Drivers/SHT_Library.hpp"
#include "I2C/I2C.hpp"
#include "stdint.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace SHT_DRVR {

// Requires bool to reset timeout value, and int of timeout. The resetTimeout
// is default false, and timeout_ms is default 500. If left unspecified, 
// default values will persist.
void RWPacket::reset(bool resetTimeout, int timeout_ms) {
    this->dataSafe = false;
    memset(this->writeBuffer, 0, sizeof(this->writeBuffer));
    memset(this->readBuffer, 0, sizeof(this->readBuffer));
    if (resetTimeout) this->timeout = timeout_ms;
}

SHT::SHT() : tag("(SHT31)"), isInit(false) {this->packet.reset();} // Inits RW 

// Requires no parameters. Writes the command in the RW Packet write
// buffer to the SHT. Returns WRITE_TIMEOUT if timed out, WRITE_FAIL
// if write failed, or WRITE_OK if successful. 
SHT_RET SHT::write() {
    esp_err_t err = i2c_master_transmit(
        this->i2cHandle,
        this->packet.writeBuffer, sizeof(this->packet.writeBuffer), 
        this->packet.timeout
    );

    this->packet.dataSafe = (err == ESP_OK);

    if (err == ESP_ERR_TIMEOUT) {
        printf("%s write Timed Out\n", this->tag);
        return SHT_RET::WRITE_TIMEOUT;
    } else if (err != ESP_OK) {
        printf("%s write Err: %s\n", this->tag, esp_err_to_name(err));
        return SHT_RET::WRITE_FAIL;
    }

    return SHT_RET::WRITE_OK;
}

// Requires the readSize. This is from 2 - 6, 2 being status reading,
// and 6 being a temperature, humidity, and checksum reading. Updates
// the RW packet read buffer upon success and returns READ_TIMEOUT if
// timed out, READ_FAIL if unsuccessful, READ_FAIL_CHECKSUM if failed
// due to unsuccessful checksum comparison, or READ_OK upon successful
// read and checksum validation.
SHT_RET SHT::read(size_t readSize) {

    esp_err_t transErr = i2c_master_transmit(
        this->i2cHandle,
        this->packet.writeBuffer, sizeof(this->packet.writeBuffer),
        this->packet.timeout
    );

    // This value was set to 20, and caused I2C issues, when changed to 
    // 50, zero issues have occured.
    vTaskDelay(pdMS_TO_TICKS(SHT_READ_DELAY)); 

    esp_err_t receiveErr = i2c_master_receive(
        this->i2cHandle,
        this->packet.readBuffer, readSize,
        this->packet.timeout
    );

    // If no errors, updates dataSafe to true.
    this->packet.dataSafe = (transErr == ESP_OK && receiveErr == ESP_OK);

    if (transErr == ESP_ERR_TIMEOUT || receiveErr == ESP_ERR_TIMEOUT) {
        printf("%s read Timed Out\n", this->tag);
        return SHT_RET::READ_TIMEOUT;

    } else if (transErr != ESP_OK || receiveErr != ESP_OK) {
        printf("%s read Error! Transmit: %s; Receive: %s\n", this->tag,
        esp_err_to_name(transErr), esp_err_to_name(receiveErr));
        return SHT_RET::READ_FAIL;
    }

    return SHT_RET::READ_OK;
}

// Requires bool dataSafe to be passed by reference to ensure data
// is read without error. Returns uint16_t value of current status
// broken down as such per pg. 13 of the datasheet:
// Bit 15 : 0 (No pending alerts), 1 (At least 1 pending alert)
// Bit 13 : 0 (Heater off), 1 (Heater on)
// Bit 11 : 0 (RH not tracking alert), 1 (RH tracking alert)
// Bit 10 : 0 (Temp not tracking alert), 1 (Temp tracking alert)
// Bit 4 : 0 (No reset detected since last clear status command), 
//         1 (Reset detected)
// Bit 1 : 0 (Last command executed successfully), 
//         1 (last command not processed)
// Bit 0 : 0 (Checksum of last write transfer was correct),
//         1 (Checksum of last write transfer failed)
// Returns 0 if data is corrupt.
uint16_t SHT::getStatus(bool &dataSafe) {
    this->packet.reset();

    this->packet.writeBuffer[0] = // MSB move into LSB
        static_cast<uint16_t>(CMD::STATUS) >> 8;

    this->packet.writeBuffer[1] = // LSB
        static_cast<uint16_t>(CMD::STATUS) & 0xFF;
  
    this->read(2); // expect 2 bytes

    uint16_t status = ( // shift bytes into LSB/MSB position.
        (this->packet.readBuffer[0] << 8) | this->packet.readBuffer[1]
        );

    dataSafe = this->packet.dataSafe;

    if (dataSafe) {
        return status;
    } else {
        return 0;
    }
}

// Requires uint8_t buffer of the read values, and the length of the
// buffer. Computes crc8 and returns its 8-bit checksum value.
uint8_t SHT::crc8(uint8_t* buffer, uint8_t length) {
    // For testing, per the datasheet pg. 14 0xBEEF is sent to the checksum
    // and the CS value is 0x92. Both the polynomial and initialization
    // crc values are also defined here.
    
    const uint8_t polynomial = 0x31;
    uint8_t crc = 0xFF;

    for (int i = length; i; --i) {
        crc ^= *buffer++;

        for (int j = 8; j; --j) {
            crc = (crc & 0x80) ? (crc << 1) ^ polynomial : (crc << 1);
        }
    }

    return crc;
}

// Requires an address to the carrier of the datasheet. Returns READ_FAIL if 
// the data is marked as corrupt, or READ_OK if successfully computed and data 
// is not corrupt.
SHT_RET SHT::computeTemps(SHT_VALS &carrier) { 
    if (!this->packet.dataSafe) return SHT_RET::READ_FAIL; // Check data

    // computes raw temp by moving LSB/MSB into the positions.
    uint16_t rawTemp = this->packet.readBuffer[0] << 8 | 
        this->packet.readBuffer[1];

    // Does the same as temp for humidity.
    uint16_t rawHum = this->packet.readBuffer[3] << 8 |
        this->packet.readBuffer[4]; // was idx 1, didnt cause error but fixed.

    // Values are computed per pg. 14 of the datasheet.
    carrier.tempF = -49 + (315.0f * rawTemp / 65535);
    carrier.tempC = -45 + (175.0f * rawTemp / 65535);
    carrier.hum = 100.0f * rawHum / 65535;
    carrier.dataSafe = true;
    return SHT_RET::READ_OK;
}

// Requires uint8_t address. Configures the I2C and adds the address to the 
// I2C bus. The default address of the SHT31 is 0x44 which has the AD pin 
// connected to logic LOW with a pulldown resistor. The address can be changed
// to 0x45 by connecting the AD pin to logic HIGH per the datasheet pg. 9.
void SHT::init(uint8_t address) {
    if (this->isInit) return; // Prevents from being re-init.

    // Init I2C and add device to service.
    Serial::I2C* i2c = Serial::I2C::get();
    i2c_device_config_t devCon = i2c->configDev(address);
    this->i2cHandle = i2c->addDev(devCon);
    this->isInit = true;
}

// Requires the start command, and the SHT value carrier. Computes
// both the temperature C/F, and humidity, which will populate to the
// carrier, along with a boolean indicating if the data is non-corrupt.
// Returns READ_FAIL, READ_FAIL_CHECKSUM, READ_TIMEOUT, or READ_OK.
SHT_RET SHT::readAll(START_CMD cmd, SHT_VALS &carrier, int timeout_ms) {
    this->packet.reset(true, timeout_ms);
    
    carrier.dataSafe = false; // Sets to true once complete.

    // Set write buffer MSB by shifting Cmd MSB by 8 and putting it in IDX0.
    this->packet.writeBuffer[0] = 
        static_cast<uint16_t>(cmd) >> 8;

    // Set write buffer LSB by &'ing it and putting it in IDX1.
    this->packet.writeBuffer[1] =
        static_cast<uint16_t>(cmd) & 0xFF;

    // Expect 6 bytes, byte 1 & 2 for temp, byte 3 for temp CS,
    // byte 4 & 5 for humidity, and byte 6 for humidity CS.
    SHT_RET status = this->read(6); 

    if (status != SHT_RET::READ_OK) {
        return status;
    }

    // Run checksums on the data to ensure they match the checksums sent 
    // by the SHT31. Upon success, computes values.
    uint8_t crcTemp = this->crc8(this->packet.readBuffer, 2);
    uint8_t crcHum = this->crc8(&this->packet.readBuffer[3], 2);

    // Compare temp/hum crc values against 
    if (this->packet.readBuffer[2] != crcTemp || 
        this->packet.readBuffer[5] != crcHum) {
            printf("%s Checksum Error\n", this->tag);
            return SHT_RET::READ_FAIL_CHECKSUM;
        }

    return this->computeTemps(carrier);
}

// Requires enable true or false. Returns WRITE_OK, WRITE_FAIL, or
// WRITE_TIMEOUT.
SHT_RET SHT::enableHeater(bool enable) {
    this->packet.reset();

    switch (enable) {
        case true:
        // Enable Heater Command.
        this->packet.writeBuffer[0] = 
            static_cast<uint16_t>(CMD::HEATER_EN) >> 8;

        this->packet.writeBuffer[1] = 
            static_cast<uint16_t>(CMD::HEATER_EN) & 0xFF;
        break;

        case false:
        // Disable Heater Command.
        this->packet.writeBuffer[0] = 
            static_cast<uint16_t>(CMD::HEATER_NEN) >> 8;

        this->packet.writeBuffer[1] = 
            static_cast<uint16_t>(CMD::HEATER_NEN) & 0xFF;
    }
    
    return this->write(); // Write command to SHT
}

// Requires bool dataSafe. Returns true if the data is safe and the heater
// is enabled. Returns false if the data is not safe, or the heater is not
// enabled. A return of true and dataSafe = true, ensures that the heater
// is enabled.
bool SHT::isHeaterEn(bool &dataSafe) {
 
    uint16_t data = this->getStatus(dataSafe); // Get the uint16_t status data.

    if (dataSafe) {
        // Move 13th bit to idx 0, return that value.
        return data >> 13 & 0x0001;
    } else {
        return false;
    }
}

// Requires no arguments. Clears the status register and will set 
// bites 15, 11, 10, and 4 to zero IAW pg. 13 of the datasheet.
// Returns WRITE_OK, WRITE_FAIL, or WRITE_TIMEOUT.
SHT_RET SHT::clearStatus() {
    this->packet.reset();

    this->packet.writeBuffer[0] = 
        static_cast<uint16_t>(CMD::CLEAR_STATUS) >> 8;

    this->packet.writeBuffer[1] =
        static_cast<uint16_t>(CMD::CLEAR_STATUS) & 0xFF;

    return this->write();
}

// Requires no arguments. Resets the device. Returns WRITE_OK,
// WRITE_FAIL, or WRITE_TIMEOUT.
SHT_RET SHT::softReset() {
    this->packet.reset();

    this->packet.writeBuffer[0] = 
        static_cast<uint16_t>(CMD::SOFT_RESET) >> 8;

    this->packet.writeBuffer[1] =
        static_cast<uint16_t>(CMD::SOFT_RESET) & 0xFF;

    return this->write();
}

}