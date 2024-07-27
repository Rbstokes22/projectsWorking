#include "I2C/I2C.hpp"
#include <cstdint>
#include <config.hpp>

// Once error handling is implemeneted after OLED, use throughout here as well as info messges,
// such as startup frequency, device connected, etc...

namespace I2C {

static uint32_t i2cFrequency{I2C_DEF_FRQ}; // standard frequency
static bool isInit{false};
static i2c_master_bus_handle_t busHandle;

// Frequency between 100000 and 400000.
bool i2c_master_init(uint32_t frequency) {
    if (isInit) return false; // Already been initialized

    // Only matters if the frequency exceeds 400k, if less than 100k,
    // the default frequency persists.

    if (frequency > 400000) {i2cFrequency = 400000;}
    else if (frequency < 100000) {i2cFrequency = 100000;}

    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.scl_io_num = GPIO_NUM_22;
    i2c_mst_config.sda_io_num = GPIO_NUM_21;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &busHandle); 

    // ADD ERROR HANDLING
    if (err != ESP_OK) {
        printf("err: %s\n", esp_err_to_name(err)); 
        return false;
    } else {
        isInit = true;
        return true;   
    }
}

// Configures the individual device by address, Uses a static frequency
// pased in the init function.
i2c_device_config_t configDev(uint8_t i2cAddr) {
    i2c_device_config_t dev_cfg = { // Do I make this a class an dev_cfg each dev?
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2cAddr,
        .scl_speed_hz = i2cFrequency,
    };

    return dev_cfg;
}

// Returns the device handle used in the i2c transmission.
i2c_master_dev_handle_t addDev(i2c_device_config_t &dev_cfg) {
    i2c_master_dev_handle_t devHandle;

    ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &dev_cfg, &devHandle));

    return devHandle;
}

}