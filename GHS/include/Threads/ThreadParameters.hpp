#ifndef THREADPARAMETERS_HPP
#define THREADPARAMETERS_HPP

#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Threads {

struct mainThreadParams {
    uint32_t delay;
    Threads::Mutex mutex;
    mainThreadParams(uint32_t delay, Messaging::MsgLogHandler &msglogerr);
};

}


#endif // THREADPARAMETERS_HPP