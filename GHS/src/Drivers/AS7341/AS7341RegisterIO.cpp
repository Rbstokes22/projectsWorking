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
// set to a value that it already set. Returns true or false.
bool AS7341basic::setBank(REG reg) {
    static uint8_t lastAddr = 0x00;
    static const uint8_t cutoff = static_cast<uint8_t>(REG::ENABLE); // 128/0x80

    // This is the buffer to send if the desired register is >= 0x80.
    uint8_t buffer[2] = {static_cast<uint8_t>(REG::REG_BANK), 0x00};
    uint8_t addr = static_cast<uint8_t>(reg);

    // Used for init only. Sets the last address to a range that is different
    // that the current register address. This will trigger a send.
    if (lastAddr == 0x00) { // First time only
        lastAddr = addr >= cutoff ? 0x00 : cutoff;
    }

    // Looks at the current register address. If it is in a different range
    // than the previous call, it will transmit to set the new register bank
    // IAW datasheet Pg.53.
    if (addr >= cutoff && lastAddr < cutoff) {

        if (this->i2c.txrxOK) {

            this->i2c.response = i2c_master_transmit(this->i2c.handle, buffer, 
                2, AS7341_TIMEOUT);

            Serial::I2C::get()->monitor(this->i2c);
        }

        lastAddr = addr; // 00000001

    } else if(addr < cutoff && lastAddr >= cutoff) {

        // Register map (pg26) says shift 3, details (pg54) say shift 4. 
        // Does not work with 4, only works with 3. 
        buffer[1] |= (1 << 3); // Sets byte to 0x08 for register below 0x80.

        if (this->i2c.txrxOK) {

            this->i2c.response = i2c_master_transmit(this->i2c.handle, buffer, 
                2, AS7341_TIMEOUT);

            Serial::I2C::get()->monitor(this->i2c);
        }

        lastAddr = addr;
    } 

    vTaskDelay(pdMS_TO_TICKS(AS7341_I2C_DELAY)); // Brief delay after write.

    // If error with device or respons is not OK, log.
    if (!this->i2c.txrxOK || this->i2c.response != ESP_OK) {

        snprintf(this->log, sizeof(this->log), "%s Register bank err. %s", 
            this->tag, esp_err_to_name(this->i2c.response));
        
        this->sendErr(this->log);
        
        return false;
    } 

    return true;
}

// Requires register and value to write. Writes value to register address.
bool AS7341basic::writeRegister(REG reg, uint8_t val) {

    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->setBank(reg)) { // Ensure bank is set. Then write to register.

        uint8_t buffer[2] = {addr, val};

        if (this->i2c.txrxOK) {

            this->i2c.response = i2c_master_transmit(this->i2c.handle, buffer, 
                2, AS7341_TIMEOUT);

            Serial::I2C::get()->monitor(this->i2c);

            // Brief delay after write.
            vTaskDelay(pdMS_TO_TICKS(AS7341_I2C_DELAY)); 

            if (this->i2c.response != ESP_OK) {

                snprintf(this->log, sizeof(this->log), 
                    "%s Register write err. %s", this->tag, 
                    esp_err_to_name(this->i2c.response));

                this->sendErr(this->log);
                return false;
            }

            return true; // Success.
        }
    }

    // Bank not set. Log captured in set bank. Default return.
    return false;
}

// Requires register and dataSafe boolean. Writes to I2C the register 
// to read, and reads the uint8_t value given back. If successful,
// changes dataSafe to true, and returns the result. Returns a uint8_t
// value, with a corresponding dataSafe boolean. 0 with a corresponding
// dataSafe = false indicates error. ATTENTION: This will be called twice,
// per read, once for the MSB and once for the LSB.
uint8_t AS7341basic::readRegister(REG reg, bool &dataSafe) {

    uint8_t addr = static_cast<uint8_t>(reg);

    if (this->setBank(reg)) { // Set bank and read from register.

        uint8_t readBuf[1]{0}; // Single byte value. Will have data populated.
        uint8_t writeBuf[1] = {addr}; // Address to send to.

        if (this->i2c.txrxOK) {

            this->i2c.response = i2c_master_transmit_receive(this->i2c.handle,
                writeBuf, sizeof(writeBuf), readBuf, sizeof(readBuf), 
                AS7341_TIMEOUT);

            Serial::I2C::get()->monitor(this->i2c);

            // Brief delay following write.
            vTaskDelay(pdMS_TO_TICKS(AS7341_I2C_DELAY)); 

            if (this->i2c.response != ESP_OK) {
                snprintf(this->log, sizeof(this->log), 
                    "%s Register Read err. %s", this->tag, 
                    esp_err_to_name(this->i2c.response));

                this->sendErr(this->log);
                dataSafe = false;

            } else {

                // Only return when data is solid.
                dataSafe = true;
                return (uint8_t)readBuf[0];
            }
        }
    } 
    
    // bank not set, logging in set bank function.
    dataSafe = false;
    return 0;
    
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
        vTaskDelay(pdMS_TO_TICKS(10)); // delay after Power ON
    }

    dataValidation = this->readRegister(reg, dataSafe); // Read what was written

    // Removes the smuxEn bit since it is cleared upon writing to avoid 
    // incorrect data validation when writing to enable smux. This is exclusive
    // to that and discovered upon trial and error.
    if (addr == 0x80) {

        uint8_t smuxEnbit = dataOut & 0b00010000; // isolate bit 4.
        uint8_t dataCorrection = dataOut - smuxEnbit; // subtracts is cor data
        if (smuxEnbit == 0x10) dataOut = dataCorrection; // if set, correct data
    }

    if (dataOut == dataValidation && dataSafe) { // If good return treu.
        if (verbose) {

            snprintf(this->log, sizeof(this->log), 
                "%s VALID. Register: %#x = %#x", this->tag, addr, 
                dataValidation);

            this->sendErr(this->log, Messaging::Levels::INFO);
        }

        return true;
        
    } else { // If bad, returns false.

        if (verbose) {

            snprintf(this->log, sizeof(this->log), 
                "%s INVALID. Register: %#x = %#x", this->tag, addr, 
                dataValidation);

            this->sendErr(this->log);
        }

        return false;
    }
}

}