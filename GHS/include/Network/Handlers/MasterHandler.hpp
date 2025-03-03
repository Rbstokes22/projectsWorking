#ifndef MASTERHAND_HPP
#define MASTERHAND_HPP

#include "UI/MsgLogHandler.hpp"

// Handles logging and features used throughout all the handlers.

namespace Comms {

class MASTERHAND { // USed only for the error handling.
    protected:
    static char log[LOG_MAX_ENTRY];
    static void sendErr(const char* msg, 
        Messaging::Levels lvl = Messaging::Levels::ERROR);

    static bool sendstrErr(esp_err_t err, const char* tag, const char* src);

    public: // None
};

}

#endif // MASTERHAND_HPP