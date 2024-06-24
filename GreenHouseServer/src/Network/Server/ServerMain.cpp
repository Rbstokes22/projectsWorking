#include "Network/NetworkWAP.h"

namespace Comms {

// This is the main controller. This will serve roughly the same page for 
// WAP_ONLY and STA_ONLY, with a few functionalities gone due to the 
// connection to Internet.
void NetMain::handleIndex() {
    if (NetMain::prevServerType == WIFI::WAP_ONLY) {
        NetMain::server.send(200, "text/html", "THIS IS WAP ONLY");
    } else if (NetMain::prevServerType == WIFI::WAP_SETUP) {
        NetMain::server.send(200, "text/html", WAPSetupPage);
    } else if (NetMain::prevServerType == WIFI::STA_ONLY) {
        NetMain::server.send(200, "text/html", "THIS IS STATION ONLY");
    }
}

}