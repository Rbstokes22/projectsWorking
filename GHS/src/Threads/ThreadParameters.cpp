#include "Threads/ThreadParameters.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetManager.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "Drivers/SHT_Library.hpp"
#include "Peripherals/Relay.hpp"
#include "Drivers/ADC.hpp"

namespace Threads {

netThreadParams::netThreadParams(uint32_t delay, 
    Comms::NetManager &netManager) : delay(delay), netManager(netManager) {}

SHTThreadParams::SHTThreadParams(uint32_t delay, SHT_DRVR::SHT &SHT) : 

    delay(delay), SHT(SHT) {}

AS7341ThreadParams::AS7341ThreadParams(uint32_t delay,
     AS7341_DRVR::AS7341basic &light, ADC_DRVR::ADC &photo) :

    delay(delay), light(light), photo(photo) {}

soilThreadParams::soilThreadParams(uint32_t delay, ADC_DRVR::ADC &soil) :

    delay(delay), soil(soil) {}

routineThreadParams::routineThreadParams(uint32_t delay, 
    Peripheral::Relay* relays, size_t relayQty) :

    delay(delay), relays(relays), relayQty(relayQty) {}

}