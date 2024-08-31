#ifndef THREADPARAMETERS_HPP
#define THREADPARAMETERS_HPP

#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Threads {

struct netThreadParams {
    uint32_t delay;
    Threads::Mutex mutex;
    netThreadParams(uint32_t delay, Messaging::MsgLogHandler &msglogerr);
};

struct periphThreadParams {
    uint32_t delay;
    Threads::Mutex mutex;
    periphThreadParams(uint32_t delay, Messaging::MsgLogHandler &msglogerr);
};

}


#endif // THREADPARAMETERS_HPP