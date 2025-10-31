#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"
#include "UI/MsgLogHandler.hpp"
#include "Threads/Mutex.hpp"
#include "Common/FlagReg.hpp"

namespace Serial {

// ATTENTION. Keep devices below 31 due to use of flag registers. This will not
// be a problem for this program now or ever. The MS byte will remain as a 
// placeholder.

#define I2C_LOG_METHOD Messaging::Method::SRL_LOG
#define I2C_MAX_DEV 8 // This is based on this programs archtecture.
#define I2C_TAG "(I2C)"
#define I2C_RESTART_RETRIES 5 // Attempts made to restart I2C.
#define I2C_FLAG_TAG "(I2C Flag)"
#define I2C_RESTART_FLAGS 3 // Number of problematic devices prompting master 
                            // restart.

#define I2C_ADDDEV_RETRIES 3 // Number of retries to add device to I2C.
#define I2C_NEW_IDX 255 // Used to flag first time init when adding device.

// Frequencies for I2C. Additional values can be used.
enum class I2C_FREQ : uint32_t {STD = 100000, FAST = 400000, SLOW = 50000};

enum class I2C_FLAG : uint8_t {OLED, AS7341, SHT, SOIL, PHOTO, LEAVE_AT_END}; // !!! Maybe Del

struct I2C_Restart {
    bool devSuccess; // All devices removed.
    bool busSuccess; // Bus successfully down.

    // Logs result of each removal attempt. Ensure that initial is 0.
    bool removed[I2C_MAX_DEV]; 
    bool retry; // Used if remove errors are detected, retries missed devices.
    I2C_Restart();
};

struct I2CPacket {
    i2c_master_dev_handle_t handle; // Device i2c handle.
    i2c_device_config_t config; // Device configuration.
    uint8_t arrayIdx; // position in allPkts array.
    esp_err_t response; // Captures response after transmission/receive
    bool txrxOK; // Ensures that I2C is up before transmission/receive.
    float errScore; // Exponentially decayed var to display sensor health.
    I2CPacket();
};

// WARNING. Ensure that all I2C packets are either static or a class variable
// that does not go out of scope, since their pointers will be stored upon 
// adding the device. allPkts will only be used during an entire i2c restart
// if the master bus is problematic, individual devices will be managed by
// their packet as required.

class I2C {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    static Threads::Mutex mtx;
    Flag::FlagReg restartFlags; // Used to flag individual device restart !!!!!!!!! maybe DEL if not used
    I2C_Restart restart;
    uint8_t devNum; // Device number from 0 - I2C_MAX_DEV
    I2CPacket* allPkts[I2C_MAX_DEV]; // Stores all i2c packet pointers.
    I2C_FREQ freq; // Frequency, STA 100kHz, FAST 400kHz, SLOW 50kHz.
    i2c_master_bus_handle_t busHandle; // i2c bus handle
    bool isInit; // Is initialized.
    I2C();
    I2C(const I2C&) = delete; // prevent copying
    I2C &operator=(const I2C&) = delete; // prevent assignment
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::CRITICAL, bool ignoreRepeat = false);
    bool restartI2CMaster();
    bool restartI2CDev(I2CPacket &pkt);


    public:
    static I2C* get();
    bool i2c_master_init(I2C_FREQ freq = I2C_FREQ::STD);
    i2c_device_config_t configDev(uint8_t i2cAddr);
    bool addDev(I2CPacket &pkt);
    bool monitor(I2CPacket &pkt);
};

}

#endif // I2C_HPP