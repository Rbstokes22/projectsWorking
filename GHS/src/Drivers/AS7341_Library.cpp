#include "Drivers/AS7341_Library.hpp"
#include "stdint.h"
#include "I2C/I2C.hpp"
#include "Config/config.hpp"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace AS7341_DRVR {

CONFIG::CONFIG(uint16_t ASTEP, uint8_t ATIME, uint8_t WTIME, bool LEDenable) : 

    ASTEP(ASTEP), ATIME(ATIME), WTIME(WTIME), LEDenable(LEDenable) {
        if (this->ASTEP > 65534) this->ASTEP = 65534;
    }

bool AS7341basic::prepRegister(REG reg) {
    uint8_t buffer[2] = {0xA9, 0x0};
    uint8_t addr = static_cast<uint8_t>(reg);
    esp_err_t err;

    if (addr < 0x80) {
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

uint8_t AS7341basic::readRegister(REG reg) {
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
        }
        this->safeData = true;
        return (uint8_t)readBuf[0];

    } else {
        this->safeData = false;
        return 0;
    }
}

void AS7341basic::validateWrite(REG reg, uint8_t dataOut, bool verbose) {
    uint8_t dataValidation{0};
    uint8_t addr = static_cast<uint8_t>(reg);

    this->writeRegister(reg, dataOut);

    if (addr == 0x80 && dataOut == 1) {
        vTaskDelay(pdMS_TO_TICKS(10)); // Power on delay
    }

    dataValidation = this->readRegister(reg);

    if (!verbose) return;

    if (dataOut == dataValidation) {
        printf("VALID. Register: %#x, is %#x\n", addr, dataValidation);
    } else {
        printf("INVALID: Register: %#x, is %#x\n", addr, dataValidation);
    }
}

AS7341basic::AS7341basic(CONFIG &conf) : safeData(false), conf(conf) {}

bool AS7341basic::init(uint8_t address) {
    I2C::I2C_RET ret = I2C::i2c_master_init(I2C_DEF_FRQ);
    uint8_t dataVal{0};
    uint8_t dataOut{0x00};

    if (ret == I2C::I2C_RET::INIT_FAIL) {
        return false;
    } 

    i2c_device_config_t devCon = I2C::configDev(address);
    this->i2cHandle = I2C::addDev(devCon);

    // Power on device
    this->validateWrite(REG::ENABLE, 0b00000001);

    // Configure device enable LED
    if (conf.LEDenable) this->validateWrite(REG::CONFIG, 0b00001000);

    // Configure ATIME (Recommended 29). Used to set integration time for
    // spectral measurements. Determines how long sensor collects light
    // before taking measurement.
    this->validateWrite(REG::ATIME, this->conf.ATIME);

    // Configure ASTEP (Recommended 599). Used to configure step time for
    // SMUX operation. Contorls timing of how long sensor waits between
    // switching between different photodiodes. Will write 2, 8-bit registers
    // equating to a 16-bit value.

    // Lower 8 bits
    uint8_t ASTEPdata = 0x00 | (this->conf.ASTEP & 0x00FF);
    this->validateWrite(REG::ASTEP_LWR, ASTEPdata);

    // Upper8 bits
    ASTEPdata = 0x00 | (this->conf.ASTEP >> 8);
    this->validateWrite(REG::ASTEP_UPR, ASTEPdata);

    // Configure WTIME (Recommended 0 or 1). Handles time between measurements.
    // This contributes to the integration time using ASTEP and ATIME.
    this->validateWrite(REG::WTIME, this->conf.WTIME);

    // NOT CONFIGURING ITIME

    // Finish enabling measurements for the device. Do not validate
    // since the SMUX bit is cleared once operation is finished.
    this->writeRegister(REG::ENABLE, 0b00011010);

    return true;
    
}

void AS7341basic::led(LED state) {
    switch (state) {
        case LED::ON:
        this->writeRegister(REG::LED, 0b10000010);
        break;

        case LED::OFF:
        this->writeRegister(REG::LED, 0b00000000);
        break;
    }
}

}
