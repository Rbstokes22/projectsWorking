#include "Threads/ThreadParameters.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetManager.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "Drivers/SHT_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "Peripherals/Relay.hpp"

namespace Threads {

netThreadParams::netThreadParams(uint32_t delay, 
    Comms::NetManager &netManager) : delay(delay), netManager(netManager) {}

SHTThreadParams::SHTThreadParams(uint32_t delay, SHT_DRVR::SHT &SHT) : 

    delay(delay), SHT(SHT) {}

AS7341ThreadParams::AS7341ThreadParams(uint32_t delay,
     AS7341_DRVR::AS7341basic &light, adc_oneshot_unit_handle_t &adc_unit) :

    delay(delay), light(light), adc_unit(adc_unit) {}

soilThreadParams::soilThreadParams(uint32_t delay, 
    adc_oneshot_unit_handle_t &adc_unit) :

    delay(delay), adc_unit(adc_unit) {}

routineThreadParams::routineThreadParams(uint32_t delay, 
    Peripheral::Relay* relays, size_t relayQty) :

    delay(delay), relays(relays), relayQty(relayQty) {}

}