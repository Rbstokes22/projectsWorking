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
    memset(this->error, 0, sizeof(this->error));
}

NetMain::~NetMain(){}

void NetMain::sendErr(const char* msg) {
    this->msglogerr.handle(Levels::ERROR, msg, Method::OLED, Method::SRL);
}

// The switch reads the 3 way switch to determine which mode it should be in.
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