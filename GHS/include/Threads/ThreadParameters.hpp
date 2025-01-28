#ifndef THREADPARAMETERS_HPP
#define THREADPARAMETERS_HPP

#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetManager.hpp"
#include "Drivers/AS7341/AS7341_Library.hpp"
#include "Drivers/SHT_Library.hpp"
#include "esp_adc/adc_oneshot.h"
#include "Peripherals/Relay.hpp"

namespace Threads {

struct netThreadParams {
    uint32_t delay;
    Threads::Mutex mutex;
    Comms::NetManager &netManager;
    Messaging::MsgLogHandler &msglogerr;

    netThreadParams(
        uint32_t delay, 
        Comms::NetManager &netManager,
        Messaging::MsgLogHandler &msglogerr);
};

struct SHTThreadParams {
    uint32_t delay;
    SHT_DRVR::SHT &SHT;
    Messaging::MsgLogHandler &msglogerr;

    SHTThreadParams(
        uint32_t delay, 
        SHT_DRVR::SHT &SHT,
        Messaging::MsgLogHandler &msglogerr);
};

struct AS7341ThreadParams {
    uint32_t delay;
    AS7341_DRVR::AS7341basic &light;
    Messaging::MsgLogHandler &msglogerr;
    adc_oneshot_unit_handle_t &adc_unit;

    AS7341ThreadParams(
        uint32_t delay,
        AS7341_DRVR::AS7341basic &light,
        Messaging::MsgLogHandler &msglogerr,
        adc_oneshot_unit_handle_t &adc_unit);
};

struct soilThreadParams {
    uint32_t delay;
    Messaging::MsgLogHandler &msglogerr;
    adc_oneshot_unit_handle_t &adc_unit;

    soilThreadParams(
        uint32_t delay,
        Messaging::MsgLogHandler &msglogerr,
        adc_oneshot_unit_handle_t &adc_unit);
};

struct routineThreadParams {
    uint32_t delay;
    Peripheral::Relay* relays;
    size_t relayQty;
    
    routineThreadParams(
        uint32_t delay, 
        Peripheral::Relay* relays, 
        size_t relayQty
        );
};

}


#endif // THREADPARAMETERS_HPP