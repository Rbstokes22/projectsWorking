#include "Network/Handlers/MasterHandler.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

char MASTERHAND::log[LOG_MAX_ENTRY]{0}; // Used in all handlers

// Requires message pointer, and level. Level is set to ERROR by default. Sends
// to the log and serial, the printed error.
void MASTERHAND::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, MHAND_LOG_METHOD);
}

// Requires error return from the sendstr functionality and the source or 
// function calling this. This file has several sendstr functions, so this
// is a wrapper that logs a sending error if it occurs. Returns true if no
// error.

// Requires error return from the sendstr functionality, the handler tag,
// the source/function calling this. The handlers have several sendstr 
// functions, at least in all functions that accept the http req as a param. 
// This is a wrapper that log a sending error if one occurs and returns the 
// proper response. Retruns true if no error, and false if error which allows 
// follow-on control.
bool MASTERHAND::sendstrErr(esp_err_t err, const char* tag, const char* src) { 

    if (err != ESP_OK) { // Only sends a log entry if string did not send.
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s http string unsent from %s", tag, src);

        MASTERHAND::sendErr(MASTERHAND::log);
    }

    return (err == ESP_OK); // true if no error, false if error.
}



}