#include "OTA/OTAupdates.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"

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

// Takes in the firmwareURL passed by the query string in the server. 
// Checks if the LAN is actively serving, and if not will default
// to the web.
void OTAhandler::update(const char* firmwareURL) {
    // Check LAN first
    if (this->checkLAN(firmwareURL)) {
        this->updateLAN(firmwareURL);
    } else {
        this->updateWEB(firmwareURL);
    }
}

// Primary error handler.
void OTAhandler::sendErr(const char* err) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR,
        err,
        Messaging::Method::SRL,
        Messaging::Method::OLED
    );
}

// Allows the client to rollback to a previous version if there
// is a firmware issue.
void OTAhandler::rollback() {
    const esp_partition_t* current = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    // Other partition
    const esp_partition_t* other = 
        (current->address == next->address) ? next : current;
    
    // ADD VALIDATION IN HERE

    esp_ota_set_boot_partition(other);
    esp_restart();
}

}