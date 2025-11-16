#include "I2C/I2C.hpp"
#include <cstdint>
#include "Config/config.hpp"
#include "driver/i2c_master.h"
#include "string.h"
#include "UI/MsgLogHandler.hpp"
#include "freertos/task.h"
#include "Peripherals/saveSettings.hpp"
#include "Common/FlagReg.hpp"
#include "xtensa/hal.h"
#include "rom/ets_sys.h"
#include "esp_private/periph_ctrl.h"
#include <atomic>

namespace Serial {

// WARNING. Issues were consistently present and during testing of i2c recovery,
// we unplugged the VCC cable from a peripheral device. Despite having an 
// attempted graceful restart, usually the entire device will restart instead.
// If a device is faulty or not present, this means it will not start up upon a 
// re-init. During this process, running averages and trends will be lost, so
// due to this exclusive problem, an external webserver will be the primary
// using this as a secondary in the event the primary goes down.

Threads::Mutex I2C::mtx(I2C_TAG); // define instance of mtx

// I2C packet will be assigned to each device class, and will follow the device
// for its life containing all required data.
I2CPacket::I2CPacket(uint32_t timeout_ms) : handle(NULL), arrayIdx(I2C_NEW_IDX), 
    allowReinit(true),
    txrxOK(false), errScore(0.0f), isRegistered(false), reConScore(0.0f),
    timeout_ms(timeout_ms), delta_ms(0.0f) { 
    
    memset(&this->config, 0, sizeof(this->config));
}

// Requires the start and end xthal_get_ccount(), the computes and sets the 
// delta in milliseconds. WARNING. Must be called for each tx and rx.
void I2CPacket::setDelta(uint32_t start, uint32_t end) {
    uint32_t delta = end - start;
    this->delta_ms = float(delta) / 
        (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000.0f);
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

// Requires no params. Removes master bus and returns true or false depending
// on success.
bool I2C::removeMaster() {

    if (!this->masterInit) return true; // True, signal the master is not init.

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    for (int i = 0; i < retries; i++) { // Iterate to remove bus.

        esp_err_t err = i2c_del_master_bus(this->busHandle);

        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), 
                "%s master not removed. Try %d / %d", 
                this->tag, i, retries - 1);

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

// NOTE: If using this function, ensure that txrxOK = false prior to call.
bool I2C::removeDevice(I2CPacket &pkt) {

    // Block, master must be init to remove devices.
    if (!this->masterInit) {
        return false;
    } else if (!pkt.isRegistered) {
        return true; // Returns true because the device is already removed.
                     // This allows a proper count of removed devices.
    }

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    for (int i = 0; i < retries; i++) { // Iterate to remove bus.

        esp_err_t err = i2c_master_bus_rm_device(pkt.handle);

        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), 
                "%s device @ addr %#x not removed. Try %d / %d", this->tag, 
                pkt.config.device_address, i, retries - 1);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(5)); // Brief delay added.
            continue; // Break into new loop.

        } else {

            pkt.isRegistered = false;
    
            snprintf(this->log, sizeof(this->log), 
                "%s device @ addr %#x removed", this->tag,
                pkt.config.device_address);

            this->sendErr(this->log, Messaging::Levels::INFO);
            return true; // Successful removal.
        }

        // Execute if all attempts failed.

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x remove fail", this->tag,
            pkt.config.device_address);

        this->sendErr(this->log);
        return false; // Unsuccessful removal.
    }
    
    return false; // Required in the event of for loop fail.
}

bool I2C::removeDevices() {

    size_t rmDevCt = 0;

    for (int i = 0; i < this->devNum; i++) {
  
        rmDevCt += this->removeDevice(*(this->allPkts[i]));
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    return (rmDevCt == this->devNum);
}

// Requires no params, reinits master. Returns true if successful or false if
// not.
bool I2C::reInitMaster() {
    return this->i2c_master_init(I2C_SET_FREQ);
 }

// Checks if the i2c bus is hanging. If yes, logs the data showing delta and
// line logic results.
void I2C::checkHang(I2CPacket &pkt) { 

    // First get the values of the I2C pins to see if either of them are pulled
    // low.
    bool SCL_Low = !gpio_get_level(I2C_SCL_PIN);
    bool SDA_Low = !gpio_get_level(I2C_SDA_PIN);

    if (SCL_Low || SDA_Low) { // If low, log problem.

        snprintf(this->log, sizeof(this->log),
            "%s bus hang Δ=%.2fms (SCL=%d SDA=%d) -> recovering",
            this->tag, pkt.delta_ms, SCL_Low, SDA_Low);

        this->sendErr(this->log, Messaging::Levels::WARNING);
    }
}

// Requires no params. Disables, resets, and reenables the I2C0 module, 
// manually takes control of the SDA and SCL pins, and toggles the SCL 9 time 
// IAW I2C protocol, to simulate SCL pulsing to allow devices to capture a full 
// byte + ACK/NACK. This should release any problematic devices, and the GPIO 
// control goes back to the I2C bus.
void I2C::recoverPins() {

    // Forcefully kill and reset all I2C0 hardware.
    periph_module_disable(PERIPH_I2C0_MODULE);
    periph_module_reset(PERIPH_I2C0_MODULE);
    periph_module_enable(PERIPH_I2C0_MODULE);

    // Ensure set to output before toggle to maintain control.
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(I2C_SCL_PIN, 1);
    gpio_set_level(I2C_SDA_PIN, 1);

    // Pulse SCL to clear any slaves.
    for (int i = 0; i < 9; i++) {
        gpio_set_level(I2C_SCL_PIN, 1);
        ets_delay_us(5); 
        gpio_set_level(I2C_SCL_PIN, 0);
        ets_delay_us(5);
    }

    // Once signal is sent, hand control back to I2C bus
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT_OUTPUT_OD);
}

bool I2C::hardResetBus(I2CPacket &pkt) {

    printf("STARTING HARD RESET\n");
    // First stop all tx/rx for each i2c device. This will be re-enabled only
    // when the devices are re-init after bus reset.
    for (uint8_t i = 0; i < this->devNum; i++) {
        this->allPkts[i]->txrxOK = false; // disable all txrx.
    }

    this->checkHang(pkt); // Will log if a hang is detected.

    pkt.reConScore += HEALTH_ERR_UNIT;

    // This will determine problematic devices. Individual packet that prompts
    // bus reset will accumulate a reconnection score, and permanently disable
    // the device, preventing it from being re-initialized or txrx.
    if (pkt.reConScore > HEALTH_RECON_BAD) {
        pkt.txrxOK = false;
        pkt.allowReinit = false;

        snprintf(this->log, sizeof(this->log),
            "%s Device @ Addr %#x has been permanently disabled\n", this->tag,
            pkt.config.device_address);

        this->sendErr(this->log); 
    } 

    // Take control of the bus and recover pins, pulsing scl.
    this->recoverPins();

    // After pins are recovered, first remove devices
    bool delDev = this->removeDevices();
    bool delMaster = false;

    // Next remove the master bus.
    if (delDev) {
        delMaster = this->removeMaster();
    }

    // Block if master unable to remove. All err handling and logging within
    // called functions.
    if (!delMaster) return false; 

    // Once pins are toggle, add master.
    bool initMaster = this->reInitMaster();

    // ATTENTION.
    // No logic below to ensure devices added back to bus equal the original
    // quantity. This is by design by flagging problematic devices to prevent
    // re-init. If devices are somehow not added back, then implement err
    // handling logic.

    if (initMaster) { // Master has been init, add back all devices.

        for (uint8_t i = 0; i < this->devNum; i++) {
            this->addDev(*(this->allPkts[i]), true);
        }
    }

    return (initMaster); // True if re-init, false if not.
}

// Requires packet. ENSURE that packet is updated before sending with the
// txrx response for analysis. Ensures that any timeouts are captured and
// checks for potential device and/or bus failures. If individual devices are
// failing, removes and adds back to master. If bus is detected, removes all
// devices, bus, and re-inits devices. Returns true based on function working
// as advertised, and false if operations are unsuccessful.
bool I2C::monitor(I2CPacket &pkt) { 

    // No mutex required here, already under mutex protection.

    // WARNING. Do not attempt to remove individual devices. Testing showed 
    // this to be extremely problematic, with the i2c bus freezing if attempt
    // while a device might be holding the SDA low. Instead take control of 
    // bus, remove all devices and bus, and re-init, keeping track of a 
    // reconnection score.

    // Check the error score and compare it against the value that flags
    // the device as unresponsive. If unresponsive attempt to reset the entire
    // bus. 
    if ((pkt.errScore > HEALTH_ERR_BAD) && (pkt.arrayIdx < I2C_MAX_DEV)) { 

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x unresponsive @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_BAD);

        this->sendErr(this->log);

        this->hardResetBus(pkt); // Attempt bus reset. Temp Block
        return false;

    } else if (pkt.errScore >= HEALTH_ERR_BAD / 2.0f) { // BECMG unresponsive

        snprintf(this->log, sizeof(this->log), 
            "%s device @ addr %#x failing @ %0.2f / %0.2f", this->tag,
            pkt.config.device_address, pkt.errScore, HEALTH_ERR_BAD);

        this->sendErr(this->log, Messaging::Levels::WARNING);
    } 

    return true; // Indicates that device is within params.
}

// Returns class instance to singleton.
I2C* I2C::get() {
    static I2C instance;
    return &instance;
}

// Requires I2C_FREQ parameter with STD (100k) or FAST (400k).
// Initializes the I2C with SCL on GPIO 22 and SDA on GPIO 21.
// Returns true of false depending on if it was init.
bool I2C::i2c_master_init(I2C_FREQ freq) {

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return false; // Block if unlocked.
    }

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
                "%s master bus not added. Try %d / %d", this->tag, i, 
                retries - 1);

            this->sendErr(this->log);
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
i2c_device_config_t I2C::configDev(uint8_t i2cAddr) { // No mtx required.

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

    Threads::MutexLock guard(this->mtx);
    if (!guard.LOCK()) {
        return false; // Block if unlocked.
    }

    // Ensure that the device number is in compliance and master has been init.
    if (this->devNum >= I2C_MAX_DEV) {
        snprintf(this->log, sizeof(this->log), "%s DEV FULL", this->tag);
        this->sendErr(this->log);
        return false;

    } else if (!this->masterInit) { // Ensures master is init before add.
        snprintf(this->log, sizeof(this->log), "%s Master unInit", this->tag);
        this->sendErr(this->log);
        return false;

    } else if (pkt.isRegistered) { // Ensure device is unregistered.
        snprintf(this->log, sizeof(this->log), "%s dev already init", 
            this->tag);

        this->sendErr(this->log, Messaging::Levels::ERROR);
        return true; // Device is already registered and working.

    } else if (!pkt.allowReinit) {
        // Do not log here, this is a blocking function only if the device
        // has been flagged as being permanently unstable.
        return false;
    }

    int retries = (I2C_ADDDEV_RETRIES > 0) ? I2C_ADDDEV_RETRIES : 1;

    // Iterate and attempt to add device to master mus.
    for (int i = 0; i < retries; i++) {

        esp_err_t err = i2c_master_bus_add_device(this->busHandle, &pkt.config,
            &pkt.handle);

        if (err != ESP_OK) {

            pkt.txrxOK = false;
            pkt.isRegistered = false;

            snprintf(this->log, sizeof(this->log), 
                "%s dev not added at addr %#x. Try %d / %d", this->tag, 
                pkt.config.device_address, i, retries - 1);

            this->sendErr(this->log);
            
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

            // WARNING. Do not zero out reConScore.
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

// ATTENTION. Keep all TX/RX centralized in this mutex protected singleton.
// This will ensure proper and centralized control required to monitor and 
// ensure maximum bus health. Each TX/RX has a brief delay. This is because
// the mutex was constantly timing out, now this forces separation between 
// calls to the I2C bus via phase desynchronization to allow for proper flow.

// Requires i2c packet, a pointer to the write buffer, and its size. Transmits
// via i2c. Returns true if successful, and false if not.
bool I2C::TX(I2CPacket &pkt, const uint8_t* writeBuf, size_t bufSize) {

    if (pkt.txrxOK) {

        ets_delay_us(300); // 0.3ms brief delay to clear mtx race problems.
        
        Threads::MutexLock guard(this->mtx);
        if (!guard.LOCK()) {
            return false; // Block if unlocked.
        }

        uint32_t start = xthal_get_ccount();

        pkt.response = i2c_master_transmit(pkt.handle, writeBuf, bufSize,
            pkt.timeout_ms);

        pkt.setDelta(start, xthal_get_ccount());

        if (pkt.response != ESP_OK) {

            pkt.errScore += HEALTH_ERR_UNIT; // Accumulate if error.
            if (pkt.errScore > HEALTH_ERR_MAX) pkt.errScore = HEALTH_ERR_MAX;

            snprintf(this->log, sizeof(this->log), "%s tx err @ addr %#x: %s", 
                this->tag, pkt.config.device_address, 
                esp_err_to_name(pkt.response));

            this->sendErr(this->log);

            this->monitor(pkt);
            
            return false; 
        }

        pkt.errScore *= HEALTH_EXP_DECAY; // Decay if no error.
        this->monitor(pkt);

        return true;
    }

    return false;
}

// Requires i2c packet, a pointer to the read buffer, and its size. Receives
// via i2c. Returns true if successful, and false if not.
bool I2C::RX(I2CPacket &pkt, uint8_t* readBuf, size_t bufSize) {

    if (pkt.txrxOK) {

        ets_delay_us(300); // 0.3ms brief delay to clear mtx race problems.
        
        Threads::MutexLock guard(this->mtx);
        if (!guard.LOCK()) {
            return false; // Block if unlocked.
        }

        uint32_t start = xthal_get_ccount();

        pkt.response = i2c_master_receive(pkt.handle, readBuf, bufSize,
            pkt.timeout_ms);

        pkt.setDelta(start, xthal_get_ccount());

        if (pkt.response != ESP_OK) {

            pkt.errScore += HEALTH_ERR_UNIT; // Accumulate if error
            if (pkt.errScore > HEALTH_ERR_MAX) pkt.errScore = HEALTH_ERR_MAX;

            snprintf(this->log, sizeof(this->log), "%s rx err @ addr %#x: %s", 
                this->tag, pkt.config.device_address,
                esp_err_to_name(pkt.response));

            this->sendErr(this->log);

            this->monitor(pkt);
            
            return false; 
        }

        pkt.errScore *= HEALTH_EXP_DECAY; // Decay if no error.
        this->monitor(pkt);

        return true;
    }

    return false;
}

// Requires i2c packet, a pointer to the write buffer, the write buf size,
// a pointer to the read buffer, and the read buf size. Transmits via i2c. 
// Returns true if successful, and false if not.
bool I2C::TXRX(I2CPacket &pkt, const uint8_t* writeBuf, size_t writeBufSize, 
    uint8_t* readBuf, size_t readBufSize) {

    if (pkt.txrxOK) {

        ets_delay_us(300); // 0.3ms brief delay to clear mtx race problems.

        Threads::MutexLock guard(this->mtx);
        if (!guard.LOCK()) {
            return false; // Block if unlocked.
        }

        uint32_t start = xthal_get_ccount();

        pkt.response = i2c_master_transmit_receive(pkt.handle, writeBuf, 
            writeBufSize, readBuf, readBufSize, pkt.timeout_ms);

        pkt.setDelta(start, xthal_get_ccount());

        if (pkt.response != ESP_OK) {

            pkt.errScore += HEALTH_ERR_UNIT; // Accumulate if error
            if (pkt.errScore > HEALTH_ERR_MAX) pkt.errScore = HEALTH_ERR_MAX;

            snprintf(this->log, sizeof(this->log), "%s txrx err @ addr %#x: %s", 
                this->tag, pkt.config.device_address,
                esp_err_to_name(pkt.response));

            this->sendErr(this->log);

            this->monitor(pkt);
     
            return false; 
        }

        pkt.errScore *= HEALTH_EXP_DECAY; // Decay if no error.
        this->monitor(pkt);

        return true;
    }

    return false;
}

// Requires i2c packet, a pointer to the write buffer, the write buf size,
// a pointer to the read buffer, the read buf size, and delay in ms. Transmits
// via i2c, introduces a delay, and then reads the response. This is designed
// for functions that must introduce delay instead of relying on tx and rx
// simultaneously. 
bool I2C::TXthenRX(I2CPacket &pkt, const uint8_t* writeBuf, size_t writeBufSize, 
    uint8_t* readBuf, size_t readBufSize, size_t delay) {

    // Acquires and locks mutex.
    bool tx = this->TX(pkt, writeBuf, writeBufSize);
    
    // Mtx unlocked after TX, introduce delay and reacquire and lock mutex
    // for RX.
    if (tx) {
        vTaskDelay(pdMS_TO_TICKS(delay));
        return this->RX(pkt, readBuf, readBufSize);
    }

    return false; // Bad tx.
}

}