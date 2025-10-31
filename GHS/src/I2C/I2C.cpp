#include "I2C/I2C.hpp"
#include <cstdint>
#include "Config/config.hpp"
#include "driver/i2c_master.h"
#include "string.h"
#include "UI/MsgLogHandler.hpp" 
#include "freertos/task.h"
#include "Peripherals/saveSettings.hpp"
#include "Common/FlagReg.hpp"

// Developed new methods for analysis. Currently stopped at the decision of 
// restarting the master or individual devices. Work that logic as well and 
// continue forward. Shouldnt be much left to do.

namespace Serial {

Threads::Mutex I2C::mtx(I2C_TAG); // define instance of mtx

// Ensures proper struct format to handle retries and successes.
I2C_Restart::I2C_Restart() : devSuccess(false), busSuccess(false), 
    retry(false) {

    memset(this->removed, 0, sizeof(this->removed));
}

// I2C packet will be assigned to each device class, and will follow the device
// for its life containing all required data.
I2CPacket::I2CPacket() : handle(NULL), arrayIdx(I2C_NEW_IDX), txrxOK(true),
    errScore(0.0f) { 
    
    memset(&this->config, 0, sizeof(this->config));
}

// Defaults frequency to 100 khz. Can be set higher when initializing the
// master.
I2C::I2C() : tag(I2C_TAG), restartFlags(I2C_FLAG_TAG), devNum(0), 
    freq(I2C_FREQ::STD), isInit(false) {

    // Zeroize all arrays and log.
    memset(this->allPkts, 0, sizeof(this->allPkts));
    memset(this->log, 0, sizeof(this->log));

    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO, true);
}

// Requires message, message level, and if repeating log analysis should be 
// ignored. Messaging default to CRITICAL, ignoreRepeat default to false.
void I2C::sendErr(const char* msg, Messaging::Levels lvl, bool ignoreRepeat) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, I2C_LOG_METHOD, 
        ignoreRepeat);
}

// References the restart struct, no params. Removes all devices from the bus
// as well as the bus itself. Returns true if successful, or false if not.
// All bookkeeping kept in the restart struct.
bool I2C::restartI2CMaster() {

    esp_err_t err;
    uint8_t removedDevices = 0;

    // Iterates the errors, adjusts the removed devices by adding 1 for each
    // device that was successfully removed previously. This allows the logic
    // check to ensure that removedDevices == the registered device number at
    // the bottom. For example if 4 of 7 devices were removed, removedDevices
    // will be set to 4, allowing the other 3 to be retried.
    for (uint8_t i = 0; i < I2C_MAX_DEV; i++) {
        removedDevices += this->restart.removed[i];
    }

    for (uint8_t i = 0; i < this->devNum; i++) {

        // Blocks during a retry ONLY due to some devices not being removed
        // properly. During iteration, checks to see if the device had been
        // removed successfully, if so, does not attempt again.
        if (this->restart.retry == true) {
            if (this->restart.removed[i] == true) continue;
        }

        err = i2c_master_bus_rm_device(this->allPkts[i]->handle);

        // Error handling includes only logging. If problems persist in the
        // future add retries. Omitted retries for the time being.
        if (err != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s Dev #%u not removed",
                this->tag, i);

            this->sendErr(this->log);
            this->restart.removed[i] = false;

        } else {
            removedDevices++;
            this->restart.removed[i] = true;
        }
    }

    // Ensure the removed devices equal the total devices. If so, delete the
    // master bus. 
    if (removedDevices == this->devNum) { // Indicates all devices removed.
        this->restart.devSuccess = true;

        err = i2c_del_master_bus(this->busHandle);

        if (err != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s BUS not removed",
                this->tag);

            this->sendErr(this->log);
            this->restart.busSuccess = false;

        } else {
            this->restart.busSuccess = true;
        }

    } else {
        this->restart.devSuccess = false;
    }

    // Upon success, clear the handles and restart the device number @ 0.
    if (this->restart.devSuccess && this->restart.busSuccess) {
        memset(this->allPkts, 0, sizeof(this->allPkts));
        this->devNum = 0;
        return true;
    }

    return false;
}

bool I2C::restartI2CDev(I2CPacket &pkt) {
    // esp_err_t err = i2c_master_bus_rm_device(this->handles[i]);
    return false;
}

// Returns a pointer to the I2C instance.
I2C* I2C::get() {
    static I2C instance;
    Threads::MutexLock(I2C::mtx);
    return &instance;
}

// Requires I2C_FREQ parameter with STD (100k) or FAST (400k).
// Initializes the I2C with SCL on GPIO 22 and SDA on GPIO 21.
// Returns true of false depending on if it was init.
bool I2C::i2c_master_init(I2C_FREQ freq) {

    if (this->isInit) return true;

    this->freq = freq; // Resets the frequency to passed init

    // Master configuration, used default GPIO pins set by ESP32.
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.scl_io_num = GPIO_NUM_22;
    i2c_mst_config.sda_io_num = GPIO_NUM_21;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;
    
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &this->busHandle);

    if (err != ESP_OK) { // Not init.
        snprintf(this->log, sizeof(this->log), "%s %s", this->tag, 
            esp_err_to_name(err));

        this->sendErr(this->log);
        return false;

    } else { // Init
        snprintf(this->log, sizeof(this->log), "%s init", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
        this->isInit = true; // Change to true to prevent re-init.
        return true;  
    }
}

// Requires the i2c address of the device. Configures device
// and returns i2c_device_config_t struct.
i2c_device_config_t I2C::configDev(uint8_t i2cAddr) {

    i2c_device_config_t dev_cfg = { // device configuration
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2cAddr,
        .scl_speed_hz = static_cast<uint32_t>(this->freq),
        .scl_wait_us = 0, // Sets default
    };

    return dev_cfg; // Returns actual configuration struct.
}

// Requires the i2c_device_config_t configuration. Adds the device to the
// i2c master bus and populates the device handle.
bool I2C::addDev(I2CPacket &pkt) {

    if (this->devNum >= I2C_MAX_DEV) {
        snprintf(this->log, sizeof(this->log), "%s DEV FULL", this->tag);
        this->sendErr(this->log, Messaging::Levels::CRITICAL);
    }

    // Add device to master bus and handle response whether good or bad.
    esp_err_t err = i2c_master_bus_add_device(this->busHandle, &pkt.config, 
        &pkt.handle);

    if (err != ESP_OK) { // Not added, prep msg.
        snprintf(this->log, sizeof(this->log), "%s ADDR %#x not added. %s", 
            this->tag, pkt.config.device_address, esp_err_to_name(err));

        this->sendErr(this->log);
        return false;

    } else { // Added, prep msg.

        // Check to see if device is a new addition vs re-init. Upon a new 
        // addition, change the array index to the device number. This is used
        // upon first init, and the packets position will always be maintained
        // at the same index.
        if (pkt.arrayIdx == I2C_NEW_IDX) {
            this->allPkts[this->devNum] = &pkt; // Save into array.
            pkt.arrayIdx = this->devNum; 
            this->devNum++; // Increment to signal successful device add.
        }

        snprintf(this->log, sizeof(this->log), "%s ADDR %#x added", 
            this->tag, pkt.config.device_address);

        this->sendErr(this->log, Messaging::Levels::INFO);
        return true;
    }
}

bool I2C::monitor(I2CPacket &pkt) {

    // Used statically in this singleton class to check the overall bus health.
    // this will be used 
    static Flag::FlagReg masterBus("(I2C_BUS_FLAGS)");

    // Error score is modified upon each response. The error score will have
    // one unit added for each timeout. It will decay approaching zero when
    // there is no timeout. The design is meant to show consecutive errors 
    // which will accumulate quickly, allowing device reset.
    pkt.errScore = (pkt.response == ESP_ERR_TIMEOUT) ?
        pkt.errScore + HEALTH_ERR_UNIT : pkt.errScore * HEALTH_EXP_DECAY;

    // Next check the error score and compare it against the value that flags
    // the device as unresponsive. If unresponsive log and set master bus flag.
    // If becoming unresponsive, just log.
    if ((pkt.errScore > HEALTH_ERR_MAX) && (pkt.arrayIdx < I2C_MAX_DEV)) { 

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x unresponsive @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_MAX);

        this->sendErr(this->log, Messaging::Levels::WARNING);

        // Set flag if unresponsive to initiate restart of device.
        masterBus.setFlag(static_cast<uint8_t>(pkt.arrayIdx));

    } else if (pkt.errScore >= HEALTH_ERR_MAX / 2.0f) { // BECMG unresponsive

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x failing @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_MAX);

        this->sendErr(this->log, Messaging::Levels::WARNING);

    } else { // Unproblematic at the moment, ensure flag is removed.

        masterBus.releaseFlag(static_cast<uint8_t>(pkt.arrayIdx));
    }

    // Now get the population count of the current flags to determine if the
    // master is the problem, or the individual device.
    uint8_t busFailures = __builtin_popcount(masterBus.getReg());

    // ATTENTION. Upon restart attempts, ensure to change the I2C packet
    // txrxOK to false, which will be used to check before any transmission.
    // This prevents a device from txrx while a restart is being attempted.

    if (busFailures >= I2C_RESTART_FLAGS) { // Restart the main bus

    } else { // Prompt the restart of individual clients.

    }


    return true; // Indicates no violations at the moment.



}

}