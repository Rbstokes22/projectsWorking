#include "MsgLogHandler.h"

namespace Messaging {

char LevelsMap[5][10]{
    "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
};

MsgLogHandler::MsgLogHandler(
    UI::IDisplay &OLED, Clock::Timer &OLEDclear, bool serialOn) : 
    OLED{OLED}, OLEDclear(OLEDclear), serialOn{serialOn}{}

void MsgLogHandler::writeSerial(Levels level, const char* message) {
    if (this->serialOn) {
        printf("%s: %s\n", 
        LevelsMap[static_cast<uint8_t>(level)], message
        );
    }
}

void MsgLogHandler::writeOLED(Levels level, const char* message) {
    char msg[128]{}; // TEST TO SEE FOR TEXT WRAPPING
    int writtenChars = snprintf(msg, sizeof(msg), "%s: %s",
    LevelsMap[static_cast<uint8_t>(level)], 
    message);

    if (writtenChars < 0 || writtenChars >= sizeof(msg)) {
        strncpy(msg, "ERROR: Message too long", sizeof(msg) - 1);
        msg[sizeof(msg) -1] = '\0';
        OLED.displayMsg(msg);
    }

    OLED.displayMsg(msg);
}

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
        writeLog(level, message); 
    }
}

// called in main loop. Checks to see if the override status 
// has been tripped. If it has, the message will be set to 
// clear in the designated seconds.
void MsgLogHandler::OLEDMessageClear(uint8_t seconds) {
    if (OLED.getOverrideStat() == true) {
    if(OLEDclear.setReminder(seconds * 1000) == true) {
      OLED.setOverrideStat(false);
    } 
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