#ifndef THREADPARAMETERS_HPP
#define THREADPARAMETERS_HPP

#include "config.hpp"
#include "Threads/Mutex.hpp"
#include "Threads.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Threads {

struct mainThreadParams {
    uint16_t delay;
    Threads::Mutex mutex;
    mainThreadParams(uint16_t delay, Messaging::MsgLogHandler &msglogerr);
};

}


#endif // THREADPARAMETERS_HPP