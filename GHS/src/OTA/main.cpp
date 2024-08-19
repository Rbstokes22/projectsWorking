#include "OTA/OTAupdates.hpp"
#include "esp_http_client.h"
#include "Network/NetMain.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"

namespace OTA {
// Static setup

UI::Display* OTAhandler::OLED = nullptr;
size_t OTAhandler::firmwareSize{0};

// Ensures that the net is actively connected to station mode.
// Returns true or false
bool OTAhandler::isConnected() {
    Comms::NetMode netMode = station.getNetType();
    bool netConnected = station.isActive();

    return (netMode == Comms::NetMode::STA && netConnected);
}

OTAhandler::OTAhandler(
    UI::Display &OLED, 
    Comms::NetMain &station,
    Messaging::MsgLogHandler &msglogerr) :

    station(station), msglogerr(msglogerr), LANhandle{0}, WEBhandle{nullptr}

    {
        OTAhandler::OLED = &OLED; 
    }


void OTAhandler::update(const char* firmwareURL) {
    // Check LAN first
    if (this->checkLAN()) {
        this->updateLAN();
    } else {
        this->updateWEB(firmwareURL);
    }
}

void OTAhandler::sendErr(const char* err) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR,
        err,
        Messaging::Method::SRL,
        Messaging::Method::OLED
    );
}

}