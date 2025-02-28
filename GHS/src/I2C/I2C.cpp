#include "I2C/I2C.hpp"
#include <cstdint>
#include "Config/config.hpp"
#include "driver/i2c_master.h"
#include "string.h"

// Once error handling is implemeneted after OLED, use throughout here as well as info messges,
// such as startup frequency, device connected, etc...

namespace Serial {

// Defaults frequency to 100 khz. Can be set higher when initializing the
// master.
I2C::I2C() : tag("(I2C)"), freq(I2C_FREQ::STD), isInit(false) {}

// Returns a pointer to the I2C instance.
I2C* I2C::get() {
    static I2C instance;
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

    if (err != ESP_OK) {
        printf("%s %s", this->tag, esp_err_to_name(err));
        return false;
    } else {
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
        .scl_wait_us = 0,
    };

    return dev_cfg;
}

// Requires the i2c_device_config_t configuration. Adds the device to the
// i2c master bus and returns the device handle.
i2c_master_dev_handle_t I2C::addDev(i2c_device_config_t &dev_cfg) {
    i2c_master_dev_handle_t devHandle;

    // Add device to master bus and handle response whether good or bad.
    esp_err_t err = i2c_master_bus_add_device(this->busHandle, &dev_cfg, 
        &devHandle);

    printf("%s ADDR:%u, %s\n", this->tag, dev_cfg.device_address, 
        esp_err_to_name(err));

    return devHandle;
}

}