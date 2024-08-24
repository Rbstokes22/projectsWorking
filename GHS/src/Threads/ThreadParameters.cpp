#include "Threads/ThreadParameters.hpp"
#include <cstdint>
#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Threads {

mainThreadParams::mainThreadParams(
    uint32_t delay, 
    Messaging::MsgLogHandler &msglogerr) :

    delay(delay), mutex(msglogerr) {}


}