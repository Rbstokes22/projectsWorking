#include "I2C/I2C.hpp"
#include <cstdint>
#include "Config/config.hpp"
#include "driver/i2c_master.h"
#include "string.h"
#include "UI/MsgLogHandler.hpp" 
#include "freertos/task.h"
#include "Peripherals/saveSettings.hpp"
#include "Common/FlagReg.hpp"

// Nearly complete at like 95%. Finish the remove and re-init inthe monitor
// class. Once complete, consider separating into some sub functions as well
// to handle certainthings, instead of having monitor extra long. Once complete
// with the project, ensure accuracy and capture of all devices and the master
// The default settings should have everything suspended and first init the master
// bus changing the init flag to true. Before any devices are added, the master
// need to be checked. The devices will then be added which should zero the err
// score, and flag reg and txtx to true allowing communication. Upon removal of 
// device, Checks should be made to ensure the master bus is active and the 
// device is registered. If it is, it can then be removed, and setting its 
// reg and txrx to false. If resetting master, ensure all devices are removed
// first. remove master first checking it is functional, and then add it back
// changing the init flag to true. Once up, each device can be added back to 
// the master, of course ensuring it is active before adding. If removing 
// individual devices the same check and flag changes must occur, and should 
// already be good.

namespace Serial {

Threads::Mutex I2C::mtx(I2C_TAG); // define instance of mtx

// I2C packet will be assigned to each device class, and will follow the device
// for its life containing all required data.
I2CPacket::I2CPacket() : handle(NULL), arrayIdx(I2C_NEW_IDX), txrxOK(false),
    errScore(0.0f), isRegistered(false) { 
    
    memset(&this->config, 0, sizeof(this->config));
}

// Defaults frequency to 100 khz. Can be set higher when initializing the
// master.
I2C::I2C() : tag(I2C_TAG), masterInit(false), devNum(0), 
    freq(I2C_FREQ::STD) {

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

bool I2C::removeMaster() {

    if (!this->masterInit) return true; // True, signal the master is not init.

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    for (int i = 0; i < retries; i++) {

        esp_err_t err = i2c_del_master_bus(this->busHandle);

        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), 
                "%s master not removed. Try %d / %d", this->tag, i, retries);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(20)); // Brief delay added.
            continue; // Break into new loop.

        } else {

            this->masterInit = false; // Flag removal from i2c master.
            
            snprintf(this->log, sizeof(this->log), 
                "%s master bus removed", this->tag);

            this->sendErr(this->log, Messaging::Levels::INFO);
            return true; // Successful removal.
        }

        // Execute if all attempts failed.

        snprintf(this->log, sizeof(this->log), "%s master bus remove fail", 
            this->tag);

        this->sendErr(this->log);
        return false; // Unsuccessful removal.
    }
    
    return false; // Required in the event of for loop fail.
}

// Requires pointer to packet.
bool I2C::removeDev(I2CPacket* pkt) {

    if (pkt == nullptr) { // Block and log if nullptr.
        snprintf(this->log, sizeof(this->log), "%s dev is nullptr", this->tag);
        this->sendErr(this->log);
        return false;

    } else if (this->masterInit) {
        snprintf(this->log, sizeof(this->log), 
            "%s dev cannot be removed from un-init master", this->tag);

        this->sendErr(this->log);
        return false;

    } else if (!pkt->isRegistered) {
        return true; // Will not remove unregistered device, returns true 
                     // indicating that the device is removed.
    }

    pkt->txrxOK = false; // Blocks use of trans/receive during removal. Will 
                         // Not be clear until device is added back to bus.

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    for (int i = 0; i < retries; i++) {

        esp_err_t err = i2c_master_bus_rm_device(pkt->handle);

        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), 
                "%s dev not removed at addr %#x. Try %d / %d", this->tag, 
                pkt->config.device_address, i, retries);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(20)); // Brief delay added.
            continue; // Break into new loop.

        } else {

            pkt->isRegistered = false; // Flag removal from i2c master.
            
            snprintf(this->log, sizeof(this->log), 
                "%s dev removed at addr %#x", this->tag, 
                pkt->config.device_address);

            this->sendErr(this->log, Messaging::Levels::INFO);
            return true; // Successful removal.
        }

        // Execute if all attempts failed.

        snprintf(this->log, sizeof(this->log), 
            "%s dev removal fail at addr %#x", this->tag, 
            pkt->config.device_address);

        this->sendErr(this->log);
        return false; // Unsuccessful removal.
    }

    return false; // Required in the event of for loop fail.
}

// Requires no params, reinits master. Returns true if successful or false if
// not.
bool I2C::reInitMaster() {return this->i2c_master_init(I2C_SET_FREQ);}

// Requires packet ptr, re-inits the device to the master bus. Returns true or 
// false depending on success.
bool I2C::reInitDev(I2CPacket *pkt) { 

    if (pkt == nullptr) { // Block if nullptr.
        snprintf(this->log, sizeof(this->log), "%s pkt is nullptr", this->log);
        this->sendErr(this->log);
    }

    return this->addDev(*pkt, true);
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

    // ATTENTION. Unlike addDev(), this can be called upon re-init, assuming
    // the masterInit flag is set to false to allow unblocking.

    if (this->masterInit) return true;

    this->freq = freq; // Resets the frequency to passed init

    // Master configuration, used default GPIO pins set by ESP32.
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.scl_io_num = GPIO_NUM_22;
    i2c_mst_config.sda_io_num = GPIO_NUM_21;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    // Iterate and add new master bus. Retry as required.
    for (int i = 0; i < retries; i++) {

        esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &this->busHandle);

        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), 
                "%s master bus not added. Try %d / %d", this->tag, i, retries);

            this->sendErr(this->log);
            this->masterInit = false;
            vTaskDelay(pdMS_TO_TICKS(20));
            continue; // Break into new loop.

        } else {
            
            snprintf(this->log, sizeof(this->log), "%s master bus added", 
                this->tag);

            this->sendErr(this->log, Messaging::Levels::INFO);
            this->masterInit = true;
            return true; // Successful addition
        }

        // Execute if all attempts failed.

        snprintf(this->log, sizeof(this->log), "%s master bus failed to init", 
            this->tag);

        this->sendErr(this->log);
        return false; // Unsuccessful addition
    }

    return false; // Required in the event of for loop fail.
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

// Requires the I2Cpacket reference, and if this is a re-init. Re-init is set
// to false by default, which applies to all new additions. Returns true or 
// false if the device is added. 
// WARNING. If removing and adding device, set reInit to true, this will 
// bypass the logic to incremement the device's position, allowing use to 
// preserve all established data. If set to false, this will potentially 
// initialize the same device twice, and WILL cause I2C issues. 
bool I2C::addDev(I2CPacket &pkt, bool reInit) {

    // Ensure that the device number is in compliance and master has been init.
    if (this->devNum >= I2C_MAX_DEV) {
        snprintf(this->log, sizeof(this->log), "%s DEV FULL", this->tag);
        this->sendErr(this->log);
        return false;

    } else if (!this->masterInit) { // Ensures master is init.
        snprintf(this->log, sizeof(this->log), "%s Master unInit", this->tag);
        this->sendErr(this->log);
        return false;

    } else if (pkt.isRegistered) { // Ensure device is unregistered.
        snprintf(this->log, sizeof(this->log), "%s dev already init", 
            this->tag);

        this->sendErr(this->log, Messaging::Levels::ERROR);
        return false;
    }

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    // Iterate and attempt to add device to master mus.
    for (int i = 0; i < retries; i++) {

        esp_err_t err = i2c_master_bus_add_device(this->busHandle, &pkt.config,
            &pkt.handle);

        if (err != ESP_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s dev not added at addr %#x. Try %d / %d", this->tag, 
                pkt.config.device_address, i, retries);

            this->sendErr(this->log);
            pkt.txrxOK = false; // Prevent use until init.

            vTaskDelay(pdMS_TO_TICKS(20)); // Brief delay added.
            continue; // Break into new loop.

        } else {

            // Check to see if device is a new addition vs re-init. Upon a new 
            // addition, change the array index to the device number. This is 
            // used upon first init, and the packets position will always be 
            // maintained at the same index. If re-init, this logic block is
            // not executed. Re-init is a built in redundancy since the packets
            // array index will be set upon init config.
            if (pkt.arrayIdx == I2C_NEW_IDX && !reInit) {
                this->allPkts[this->devNum] = &pkt; // Save into array.
                pkt.arrayIdx = this->devNum; 
                this->devNum++; // Increment to signal successful device add.
            }

            pkt.errScore = 0.0f; // Reset to 0 on new addition.
            pkt.txrxOK = true; // Set OK to transmit and receive.
            pkt.isRegistered = true; // Shows device is registered with master.
            
            snprintf(this->log, sizeof(this->log), "%s dev added at addr %#x", 
                this->tag, pkt.config.device_address);

            this->sendErr(this->log, Messaging::Levels::INFO);
            return true; // Successful removal.
        }

        // Execute if all attempts failed.

        snprintf(this->log, sizeof(this->log), 
            "%s dev addition fail at addr %#x", this->tag, 
            pkt.config.device_address);

        this->sendErr(this->log);
        return false; // Unsuccessful removal.
    }

    return false; // Required in the event of for loop fail.
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

    // Ensure error score does not exceed health max.
    if (pkt.errScore > HEALTH_ERR_MAX) pkt.errScore = HEALTH_ERR_MAX;

    // Next check the error score and compare it against the value that flags
    // the device as unresponsive. If unresponsive log and set master bus flag.
    // If becoming unresponsive, just log.
    if ((pkt.errScore > HEALTH_ERR_BAD) && (pkt.arrayIdx < I2C_MAX_DEV)) { 

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x unresponsive @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_BAD);

        this->sendErr(this->log, Messaging::Levels::WARNING);

        // Set flag if unresponsive to initiate restart of device.
        masterBus.setFlag(static_cast<uint8_t>(pkt.arrayIdx));

    } else if (pkt.errScore >= HEALTH_ERR_BAD / 2.0f) { // BECMG unresponsive

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x failing @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_BAD);

        this->sendErr(this->log, Messaging::Levels::WARNING);

    } else { // Unproblematic at the moment, ensure flag is removed.

        masterBus.releaseFlag(static_cast<uint8_t>(pkt.arrayIdx));
    }

    // Now get the population count of the current flags to determine if the
    // master is the problem, or the individual device.
    int busFailures = __builtin_popcount(masterBus.getReg());

    // ATTENTION. Upon restart attempts, ensure to change the I2C packet
    // txrxOK to false, which will be used to check before any transmission.
    // This prevents a device from txrx while a restart is being attempted.

    if (busFailures >= I2C_RESTART_FLAGS) { // Restart the main bus

        size_t totalRemoved = 0;

        // Remove devices first.
        for (uint8_t i = 0; i < this->devNum; i++) {
            totalRemoved += this->removeDev(this->allPkts[i]);
        }

        if (totalRemoved != this->devNum) { // Attempt one more iteration

            // Reset, since we will reiterate all packets, and packets that 
            // have been successfully registered will return true allowing
            // capture of all removed devices.
            totalRemoved = 0; 

            for (uint8_t i = 0; i < this->devNum; i++) {
                totalRemoved += this->removeDev(this->allPkts[i]);
            }

            if (totalRemoved != this->devNum) {
                snprintf(this->log, sizeof(this->log), 
                    "%s I2C devices unable to be removed will restart system",
                    this->tag);

                this->sendErr(this->log);
                NVS::settingSaver::get()->saveAndRestart();
                return false; // Should not be executed due to restart.
            }
        } 

        bool master = this->removeMaster();

        if (master) {
            master = this->reInitMaster();

            if (master) {

                for (uint8_t i = 0; i < this->devNum; i++) {
                    this->reInitDev(this->allPkts[i]);
                }
            }
        }

    } else { // Prompt the restart of individual clients.

        // Analyze the master bus at this point to flag the actual unreponsive packets before removing and reinit

        bool removed = this->removeDev(&pkt);
        if (removed) this->reInitDev(&pkt);
        return true;

    }


    return true; // Indicates no violations at the moment.
}

}