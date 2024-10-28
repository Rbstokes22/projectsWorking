#include "Threads/ThreadParameters.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetManager.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "Drivers/DHT_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "Peripherals/Relay.hpp"

namespace Threads {

netThreadParams::netThreadParams(
    uint32_t delay, 
    Comms::NetManager &netManager,
    Messaging::MsgLogHandler &msglogerr) :

    delay(delay), mutex(msglogerr), netManager(netManager), 
    msglogerr(msglogerr) {}

DHTThreadParams::DHTThreadParams(
    uint32_t delay, 
    DHT_DRVR::DHT &dht,
    Messaging::MsgLogHandler &msglogerr) :

    delay(delay), dht(dht), msglogerr(msglogerr) {}

AS7341ThreadParams::AS7341ThreadParams(
    uint32_t delay,
    AS7341_DRVR::AS7341basic &light,
    Messaging::MsgLogHandler &msglogerr,
    adc_oneshot_unit_handle_t &adc_unit) :

    delay(delay), mutex(msglogerr), light(light),
    msglogerr(msglogerr), adc_unit(adc_unit) {}

soilThreadParams::soilThreadParams(
    uint32_t delay,
    Messaging::MsgLogHandler &msglogerr,
    adc_oneshot_unit_handle_t &adc_unit) :

    delay(delay), msglogerr(msglogerr), adc_unit(adc_unit) {}

relayThreadParams::relayThreadParams(
    uint32_t delay, 
    Peripheral::Relay* relays,
    size_t relayQty) :

    delay(delay), relays(relays), relayQty(relayQty) {}

}