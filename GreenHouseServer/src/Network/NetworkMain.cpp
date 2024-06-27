#include "Network/NetworkMain.h"

namespace Comms {

// STATIC SETUP
WebServer NetMain::server(80);
WIFI NetMain::prevServerType{WIFI::NO_WIFI};
bool NetMain::isServerRunning{false};
bool NetMain::MDNSrunning{false};
char NetMain::ST_SSID[static_cast<int>(IDXSIZE::SSID)]{};
char NetMain::ST_PASS[static_cast<int>(IDXSIZE::PASS)]{};
char NetMain::phone[static_cast<int>(IDXSIZE::PHONE)]{};
bool NetMain::isMainServerSetup{false}; // Allows a single setup

NetMain::NetMain(Messaging::MsgLogHandler &msglogerr) : msglogerr(msglogerr) {}

NetMain::~NetMain(){}

// The send error is used during the connection to the station to limit some of the 
// verbosity on the screen. Rather than each step having a statement, it just
// sends the error here, since they are all common to eachother.
void NetMain::sendErr(const char* msg) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::OLED, 
        Messaging::Method::SRL);
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