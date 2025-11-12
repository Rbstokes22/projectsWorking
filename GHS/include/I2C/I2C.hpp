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

// WARNING. I am not sure yet, but removing two devices from 5 for example,
// will prompt a master restart if a single device fails, Set to the number
// of devices which are sure to indicate a problem, and adjust if needed.
#define I2C_RESTART_FLAGS 5 // Number of problematic devices prompting master 
                            // restart.

#define I2C_ADDDEV_RETRIES 3 // Number of retries to add device to I2C.
                             // Used for master and devices.

#define I2C_NEW_IDX 255 // Used to flag first time init when adding device.

// Frequencies for I2C. Additional values can be used.
enum class I2C_FREQ : uint32_t {STD = 100000, FAST = 400000, SLOW = 50000};

// I2C Frequency used in the scope of this program unless otherwise specificed
#define I2C_SET_FREQ I2C_FREQ::SLOW

struct I2CPacket {
    i2c_master_dev_handle_t handle; // Device i2c handle.
    i2c_device_config_t config; // Device configuration.
    uint8_t arrayIdx; // position in allPkts array.
    bool allowReinit; // Allows device to be re-init.
    esp_err_t response; // Captures response after transmission/receive
    bool txrxOK; // Ensures that I2C is up before transmission/receive.
    float errScore; // Exponentially decayed var to display sensor health.
    bool isRegistered; // Flags if device is currently active or not.
    float reConScore; // Reconnect score to disable device if very problematic.
    uint32_t timeout_ms; // Timeout for the device i2c txrx
    float delta_ms; // millis used for txrx.
    void setDelta(uint32_t start, uint32_t end); // Computes & sets delta_ms 
    I2CPacket(uint32_t timeOut_MS);
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
    bool masterInit; // Flag to establish if the master is up or down.
    uint8_t devNum; // Device number from 0 - I2C_MAX_DEV
    I2CPacket* allPkts[I2C_MAX_DEV]; // Stores all i2c packet pointers.
    I2C_FREQ freq; // Frequency, STA 100kHz, FAST 400kHz, SLOW 50kHz.
    i2c_master_bus_handle_t busHandle; // i2c bus handle
    I2C();
    I2C(const I2C&) = delete; // prevent copying
    I2C &operator=(const I2C&) = delete; // prevent assignment
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::CRITICAL, bool ignoreRepeat = false);

    // The below functions are used only to remove and reinit devices that
    // have already been added.
    bool removeMaster();
    bool removeDevices();
    bool reInitMaster();
    void checkHang(I2CPacket &pkt);
    void recoverPins();
    bool hardResetBus(I2CPacket &pkt);

    public:
    static I2C* get();
    bool i2c_master_init(I2C_FREQ freq = I2C_FREQ::STD);
    i2c_device_config_t configDev(uint8_t i2cAddr);
    bool addDev(I2CPacket &pkt, bool reInit = false);
    bool monitor(I2CPacket &pkt);
    bool TX(I2CPacket &pkt, const uint8_t* writeBuf, size_t bufSize);
    bool RX(I2CPacket &pkt, uint8_t* readBuf, size_t bufSize);
    bool TXRX(I2CPacket &pkt, const uint8_t* writeBuf, size_t writeBufSize, 
        uint8_t* readBuf, size_t readBufSize);
    bool TXthenRX(I2CPacket &pkt, const uint8_t* writeBuf, size_t writeBufSize, 
        uint8_t* readBuf, size_t readBufSize, size_t delay);
};

}

#endif // I2C_HPP