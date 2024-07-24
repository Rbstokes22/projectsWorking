#include "Threads/ThreadParameters.hpp"

namespace Threads {

mainThreadParams::mainThreadParams(
    uint32_t delay, 
    Messaging::MsgLogHandler &msglogerr) :

    delay(delay), mutex(msglogerr) {}


}