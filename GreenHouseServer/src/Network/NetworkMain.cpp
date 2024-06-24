#include "Network/NetworkMain.h"

namespace Comms {

// STATIC VARIABLE INIT
WebServer NetMain::server(80);
WIFI NetMain::prevServerType{WIFI::NO_WIFI};
bool NetMain::isServerRunning{false};
bool NetMain::MDNSrunning{false};
char NetMain::ST_SSID[32]{};
char NetMain::ST_PASS[64]{};
char NetMain::phone[15]{};
bool NetMain::isMainServerSetup{false}; // Allows a single setup


NetMain::NetMain(Messaging::MsgLogHandler &msglogerr) :
    msglogerr(msglogerr) {

    // Initialize the arrays with null chars
    memset(this->error, 0, sizeof(this->error));
}

NetMain::~NetMain(){}

// This is used in conjunction with the error array. The OLED accepts an error,
// but its reset time is 
void NetMain::appendErr(const char* msg) {
    size_t remaining = sizeof(this->error) - strlen(this->error) - 1;

    strncat(this->error, msg, remaining);

    if (remaining == 0) {
        this->msglogerr.handle(
            Levels::WARNING,
            "Errors exceeding buffer length",
            Method::SRL
        );
    }
}

WIFI wifiModeSwitch() {
    uint8_t WAP = digitalRead(static_cast<int>(NETPIN::WAPswitch));
    uint8_t STA = digitalRead(static_cast<int>(NETPIN::STAswitch));  

    if (!WAP && STA) { // WAP Exclusive
        return WIFI::WAP_ONLY;
    } else if (WAP && STA) { // Middle position, WAP Program mode for STA
        return WIFI::WAP_SETUP;
    } else if (!STA && WAP) { // Station mode reading from NVS
        return WIFI::STA_ONLY;
    } else {
        return WIFI::NO_WIFI; 
    }
}

}