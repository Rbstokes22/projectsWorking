#include "Drivers/AS7341_Library.hpp"
#include "stdint.h"
#include "I2C/I2C.hpp"
#include "Config/config.hpp"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace AS7341_DRVR {

void AS7341basic::writeRegister(REG reg, uint8_t val) {
    esp_err_t err;
    uint8_t buffer[2] = {static_cast<uint8_t>(reg), val};

    err = i2c_master_transmit(this->i2cHandle, buffer, 2, -1);

    if (err != ESP_OK) {
        printf("Err writing register: %s\n", esp_err_to_name(err));
    }
}

void AS7341basic::readRegister(REG reg, uint8_t* data, size_t size) {
    esp_err_t err;
    uint8_t buffer[1] = {static_cast<uint8_t>(reg)};

    err = i2c_master_transmit(this->i2cHandle, buffer, 1, -1);

    if (err != ESP_OK) {
        printf("Err writing register: %s\n", esp_err_to_name(err));
    }

    err = i2c_master_receive(this->i2cHandle, data, size, -1);

    if (err != ESP_OK) {
        printf("Err reading register: %s\n", esp_err_to_name(err));
    }
}

AS7341basic::AS7341basic() {}

bool AS7341basic::init(uint8_t address) {
    printf("Called init\n");
    if (I2C::i2c_master_init(I2C_DEF_FRQ)) {
        i2c_device_config_t devCon = I2C::configDev(address);
        this->i2cHandle = I2C::addDev(devCon);

        // Power on device
        this->writeRegister(REG::ENABLE, 0b00000001);
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay while starting

        uint8_t regRead[1]{0};
        this->readRegister(REG::ENABLE, regRead, sizeof(regRead));
        printf("!!!!!!!!!!Reg Read: %d\n", regRead[0]);

        return true;
    } else {
        printf("Did not init i2c\n");
        return false;
    }
}

}
