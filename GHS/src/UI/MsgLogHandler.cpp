#include "UI/MsgLogHandler.hpp"
#include <cstdint>
#include "string.h"
#include "Common/Timing.hpp"
#include "freertos/FreeRTOS.h"

namespace Messaging {

const char LevelsMap[5][10]{
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
};

uint32_t MsgLogHandler::millis() {
  return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

MsgLogHandler::MsgLogHandler(
    UI::IDisplay &OLED, uint8_t msgClearSeconds, bool serialOn) : 

    OLED{OLED}, serialOn{serialOn}, msgClearTime(0), 
    msgClearSeconds{msgClearSeconds}, isMsgReset{true} {}

// Serial handler, disabled upon production but can be turned on for 
// troubleshooting.
void MsgLogHandler::writeSerial(Levels level, const char* message) {
    if (this->serialOn) {
        printf("%s: %s\n", 
        LevelsMap[static_cast<uint8_t>(level)], message);
    }
}

// The OLED is the primary interface in an operational status.
void MsgLogHandler::writeOLED(Levels level, const char* message) {
    
    // OLED has 200 char capacity @ 5x7, and is set max for the
    // buffer size. Errors will always display in 5x7 font.
    char msg[static_cast<int>(UI::UIvals::OLEDCapacity)]{}; 

    int writtenChars = snprintf(msg, sizeof(msg), "%s: %s",
    LevelsMap[static_cast<uint8_t>(level)], 
    message);

    if (writtenChars < 0 || writtenChars >= sizeof(msg)) {
        strncpy(msg, "ERROR: Message too long", sizeof(msg) - 1);
        msg[sizeof(msg) -1] = '\0';
    }

    // Displays the message and sets timer to clear the last message.
    OLED.displayMsg(msg); 

    this->msgClearTime = (this->millis() + (this->msgClearSeconds * 1000)); 
    this->isMsgReset = false; // Allows OLED display.
}

// LOG HANDLING
void MsgLogHandler::writeLog(Levels level, const char* message) {
    // BUILD
}

void MsgLogHandler::prepMessage(Method method, Levels level, const char* message) {
    switch(method) {
        case Method::SRL:
        writeSerial(level, message); break;
        case Method::OLED:
        writeOLED(level, message); break;
        case Method::LOG:
        writeLog(level, message); break;
        case Method::NONE:;
        // do nothing, used to specify parameters for meth 2 & 3.
    }
}

// Each time a new message is sent to the OLED using the logmsgerr, the 
// isMsgReset will be set to false, which will trigger a clearning of the 
// displayed message to the OLED.
void MsgLogHandler::OLEDMessageCheck() {
    if (this->millis() >= this->msgClearTime && this->isMsgReset == false) { 
        OLED.setOverrideStat(false);
        this->isMsgReset = true;
    }
}

void MsgLogHandler::handle(
    Levels level, const char* message,
    Method method1, Method method2, Method method3
    ) {

    Method methods[3]{method1, method2, method3};
    for (int i = 0; i < 3; i++) {
        prepMessage(methods[i], level, message);
    }
}

}