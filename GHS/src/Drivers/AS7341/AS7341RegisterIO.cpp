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

bool AS7341basic::prepRegister(REG reg) {
    uint8_t buffer[2] = {static_cast<uint8_t>(REG::REG_ACCESS), 0x00};
    uint8_t addr = static_cast<uint8_t>(reg);
    esp_err_t err;

    if (addr < static_cast<uint8_t>(REG::ENABLE)) {
        buffer[1] = 0x10;
    }

    err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);

    if (err != ESP_OK) {
        printf("Err config register: %s\n", esp_err_to_name(err));
        return false;
    } 

    return true;
}

void AS7341basic::writeRegister(REG reg, uint8_t val) {
    esp_err_t err;
    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->prepRegister(reg)) {
        uint8_t buffer[2] = {addr, val};
        err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);

        if (err != ESP_OK) {
            printf("Err writing register: %s\n", esp_err_to_name(err));
        }
    };
}

uint8_t AS7341basic::readRegister(REG reg, bool &dataSafe) {
    esp_err_t err;
    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->prepRegister(reg)) {
        uint8_t readBuf[1]{0};
        uint8_t writeBuf[1] = {addr};

        err = i2c_master_transmit_receive(
            this->i2cHandle,
            writeBuf, sizeof(writeBuf),
            readBuf, 1, -1
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

bool AS7341basic::validateWrite(REG reg, uint8_t dataOut, bool verbose) {
    uint8_t dataValidation{0};
    uint8_t addr = static_cast<uint8_t>(reg);
    bool dataSafe{false};

    this->writeRegister(reg, dataOut);

    if (addr == 0x80 && dataOut == 1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // delay after PON
    }

    dataValidation = this->readRegister(reg, dataSafe);

    // Removes the smuxEn bit since it is cleared upon writing to avoid 
    // Incorrect data validation.
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