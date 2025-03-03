#include "Network/Handlers/MasterHandler.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

char MASTERHAND::log[LOG_MAX_ENTRY]{0}; // USed in all handlers

// Requires message pointer, and level. Level is set to ERROR by default. Sends
// to the log and serial, the printed error.
void MASTERHAND::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, 
        Messaging::Method::SRL_LOG);
}

// Requires error return from the sendstr functionality and the source or 
// function calling this. This file has several sendstr functions, so this
// is a wrapper that logs a sending error if it occurs. Returns true if no
// error.
bool MASTERHAND::sendstrErr(esp_err_t err, const char* tag, 
        const char* src) { 

    if (err != ESP_OK) { // Only sends a log entry if string did not send.
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s http string unsent from %s", tag, src);

        MASTERHAND::sendErr(MASTERHAND::log);
    }

    return (err == ESP_OK); // true if no error, false if error.
}



}