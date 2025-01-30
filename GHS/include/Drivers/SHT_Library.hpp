#ifndef SHT_LIBRARY_HPP
#define SHT_LIBRARY_HPP

#include "I2C/I2C.hpp"
#include "stdint.h"

// Datasheet URL: https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf

namespace SHT_DRVR {

#define DEF_READ_TIMEOUT 500 // 500 millis is the default timeout

// SHT values to include floats for tempF, tempC and hum as well as a bool
// dataSafe to ensure data is good to use.
struct SHT_VALS {
    float tempF;
    float tempC;
    float hum;
    bool dataSafe;
};

// SHT read and write packet. 
struct RWPacket {
    bool dataSafe; // Updated upon successful read/write and checksum.
    uint8_t writeBuffer[2]; // Contains the MSB and LSB of the command.
    uint8_t readBuffer[6]; // Data read from i2c to this buffer
    int timeout; // timeout in ms
    void reset(bool resetTimeout = false, int timeout_ms = DEF_READ_TIMEOUT); 
};

// Return types for the SHT31.
enum class SHT_RET : uint8_t {
    READ_OK, READ_FAIL, READ_FAIL_CHECKSUM, READ_TIMEOUT,
    WRITE_OK, WRITE_FAIL, WRITE_TIMEOUT
};

// Start command, single shot mode. This is the only mode incorporated
// into this driver. The commands include a scl time stretch enable or 
// not-enable, followed by LOW, MED, or HIGH repeatability to increase
// accuracy. Reference is pg. 10 of the datasheet.
enum class START_CMD : uint16_t {
    STRETCH_HIGH_REP = 0x2C06, STRETCH_MED_REP = 0x2C0D,
    STRETCH_LOW_REP = 0x2C10, NSTRETCH_HIGH_REP = 0x2400,
    NSTRETCH_MED_REP = 0x240B, NSTRETCH_LOW_REP = 0x2416
};

// Additional commands to enable different functionalities
// of the SHT31.
enum class CMD : uint16_t {
    SOFT_RESET = 0x30A2, // Resets the serial interface
    HEATER_EN = 0x306D, // Enable the heater
    HEATER_NEN = 0x3066, // Disable the heater
    STATUS = 0xF32D, // Read out of status register
    CLEAR_STATUS = 0x3041 // Clear the status register

};

class SHT {
    private:
    i2c_master_dev_handle_t i2cHandle;
    bool isInit;
    RWPacket packet;
    SHT_RET write();
    SHT_RET read(size_t readSize);
    uint16_t getStatus(bool &dataSafe);
    uint8_t crc8(uint8_t* buffer, uint8_t length);
    SHT_RET computeTemps(SHT_VALS &carrier);

    public:
    SHT();
    void init(uint8_t address); 
    SHT_RET readAll(
        START_CMD cmd, SHT_VALS &carrier, int timeout_ms = DEF_READ_TIMEOUT
        );
        
    SHT_RET enableHeater(bool enable);
    bool isHeaterEn(bool &dataSafe);
    SHT_RET clearStatus();
    SHT_RET softReset();
};

}

#endif // SHT_LIBRARY_HPP