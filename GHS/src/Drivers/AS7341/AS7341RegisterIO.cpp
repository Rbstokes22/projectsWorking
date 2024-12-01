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

// Requires register. Checks if the previous bank set and if the new
// register range is different than the previous, adjusts the byte
// value and writes to register. This prevents the bank from being 
// set to a value that exists. Returns true or false.
bool AS7341basic::setBank(REG reg) {
    static uint8_t lastAddr = 0x00;
    static uint8_t cutoff = static_cast<uint8_t>(REG::ENABLE);

    // This is the buffer to send if register is >= 0x80.
    uint8_t buffer[2] = {static_cast<uint8_t>(REG::REG_BANK), 0x00};
    uint8_t addr = static_cast<uint8_t>(reg);
    esp_err_t err = ESP_OK;

    // Initial set for first call to trigger send. Sets the last address
    // to a range that is different than the current address.
    if (lastAddr == 0x00 && addr >= cutoff) {
        lastAddr = 0x00;
    } else if (lastAddr == 0x00 && addr < cutoff) {
        lastAddr = cutoff;
    }

    // Register map says shift 3, details say shift 4. Does not work with
    // 4, only works with 3.
    if (addr >= cutoff && lastAddr < cutoff) {
        err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);
        lastAddr = addr;
    } else if(addr < cutoff && lastAddr >= cutoff) {
        buffer[1] |= (1 << 3); // Sets byte to 0x08 for register below 0x80.
        err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);
        lastAddr = addr;
    } 

    if (err != ESP_OK) {
        printf("Err config register: %s\n", esp_err_to_name(err));
        return false;
    } 

    return true;
}

// Requires register and value to write. Writes value to register address.
bool AS7341basic::writeRegister(REG reg, uint8_t val) {
    esp_err_t err;
    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->setBank(reg)) {
        uint8_t buffer[2] = {addr, val};
        err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);

        if (err != ESP_OK) {
            printf("Err writing register: %s\n", esp_err_to_name(err));
            return false;
        }

    } else {
        return false;
    }

    return true;
}

// Requires register and dataSafe boolean. Writes to I2C the register 
// to read, and reads the uint8_t value given back. If successful,
// changes dataSafe to true, and returns the result. Returns a uint8_t
// value, with a corresponding dataSafe boolean. 0 with a corresponding
// dataSafe = false indicates error.
uint8_t AS7341basic::readRegister(REG reg, bool &dataSafe) {
    esp_err_t err;
    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->setBank(reg)) {
        uint8_t readBuf[1]{0}; // Single byte value.
        uint8_t writeBuf[1] = {addr}; // Address to send to.

        err = i2c_master_transmit_receive(
            this->i2cHandle,
            writeBuf, sizeof(writeBuf),
            readBuf, sizeof(readBuf), -1
        );

        if (err != ESP_OK) {
            printf("Register Read err: %s\n", esp_err_to_name(err));
            dataSafe = false;
        } else {
            dataSafe = true;
        }

        return (uint8_t)readBuf[0];

    } else {
        dataSafe = false;
        return 0;
    }
}

// Requires register, data to write, and verbose is set to true. Writes to 
// the register, and then reads the register to ensure that the read value
// equals the value that was written. Returns true or false if successful.
bool AS7341basic::validateWrite(REG reg, uint8_t dataOut, bool verbose) {
    uint8_t dataValidation{0};
    uint8_t addr = static_cast<uint8_t>(reg);
    bool dataSafe{false};

    this->writeRegister(reg, dataOut);

    // Handler for power on only.
    if (addr == 0x80 && dataOut == 1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // delay after PON
    }

    dataValidation = this->readRegister(reg, dataSafe);

    // Removes the smuxEn bit since it is cleared upon writing to avoid 
    // Incorrect data validation when writing to enable smux.
    if (addr == 0x80) {
        uint8_t smuxEnbit = dataOut & 0b00010000;
        uint8_t dataCorrection = dataOut - smuxEnbit;
        if (smuxEnbit == 0x10) dataOut = dataCorrection;
    }

    if (dataOut == dataValidation && dataSafe) {
        if (verbose) {
            printf("VALID. Register: %#x, is %#x\n", addr, dataValidation);
        }
        return true;
        
    } else {
        if (verbose) {
            printf("INVALID: Register: %#x, is %#x\n", addr, dataValidation);
        }
        return false;
    }
}

}