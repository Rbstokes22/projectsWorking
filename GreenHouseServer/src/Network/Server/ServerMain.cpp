#include "Network.h"

void Net::handleIndex() {
    if (this->prevServerType == WAP_ONLY) {
        server.send(200, "text/html", "THIS IS WAP ONLY");
    } else if (this->prevServerType == WAP_SETUP) {
        server.send(200, "text/html", WAPsetup);
    } else if (this->prevServerType == STA_ONLY) {
        server.send(200, "text/html", "THIS IS STATION ONLY");
    }
}