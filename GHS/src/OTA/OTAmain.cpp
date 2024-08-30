#include "OTA/OTAupdates.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

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

    station(station), msglogerr(msglogerr), OTAhandle{0}, LANhandle{0}, WEBhandle{nullptr} // DELETE

    {
        OTAhandler::OLED = &OLED; 
    }

// NOTES
bool OTAhandler::update(const char* firmwareURL, bool isLAN) {

    switch(isLAN) {
        case true:
        if (this->checkLAN(firmwareURL)) {
            this->updateLAN(firmwareURL);
            return true;
        }
        break;

        case false:
        if (this->updateWEB(firmwareURL)) {
            return true;
        }
    }

    return false;
}

// Pings the index page of the local server to check if connected.
// Returns true of false depending on connection.
bool OTAhandler::checkLAN(const char* firmwareURL) {
    esp_err_t err;
    this->config.url = firmwareURL;
    this->client = esp_http_client_init(&this->config);

    if ((err = esp_http_client_open(this->client, 0)) != ESP_OK) {
        printf("Unable to open connection: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(this->client);
        return false;
    } else {
        esp_http_client_close(this->client);
        esp_http_client_cleanup(this->client);
        return true;
    }
}

void OTAhandler::updateLAN(const char* firmwareURL) {

    const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);

    char label[20]{0}; // Gets label name either app0 or app1
    strcpy(label, part->label);

    if (part == NULL) {
        this->sendErr("No OTA partitition found");
        return;
    }

    // Checks for an active station connection.
    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return;
    }

    auto cleanup = [this](){
        if (esp_http_client_close(this->client) != ESP_OK) {
            this->sendErr("Unable to close client or never opened");
        }

        if (esp_http_client_cleanup(this->client) != ESP_OK) {
            this->sendErr("Unable to cleanup client");
        }
    };

    char sigURL[100]{0};
    sprintf(sigURL, "%s/%s", firmwareURL, "sigUpdate");

    // config and initialize http client
    this->config.url = sigURL;
    this->client = esp_http_client_init(&this->config);

    if (!writeSignature(sigURL, label)) {
        cleanup();
        this->sendErr("Unable to write signature");
        return;
    }

    char firmURL[100]{0};
    sprintf(firmURL, "%s/%s", firmwareURL, "FWUpdate");

    // config and initialize http client
    this->config.url = firmURL;
    this->client = esp_http_client_init(&this->config);

    if (!writeFirmware(firmURL)) {
        cleanup();
        this->sendErr("Unable to write firmware");
        return;
    }

    cleanup();
}

bool OTAhandler::writeSignature(const char* sigURL, const char* label) {
    esp_err_t err;
    size_t writeSize{0};
    char filepath[35]{0};
    uint8_t buffer[265]{0}; // Used to read the signature shouldnt exceed 260

    sprintf(filepath, "/spiffs/%sfirmware.sig", label); // either app0 or app1
    
    if ((err = esp_http_client_open(this->client, 0)) != ESP_OK) {
        this->sendErr("Unable to open connection");
        return false;
    }

    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        this->sendErr("Invalid content length");
        return false;
    }

    int dataRead = esp_http_client_read(this->client, (char*)buffer, sizeof(buffer));
    if (dataRead <= 0) {
        this->sendErr("Error reading signature Data");
        return false;
    }

    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        printf("Unable to open file: %s\n", filepath);
    }

    printf("Writing to filepath: %s\n", filepath); 
    writeSize = fwrite(filepath, 1, sizeof(buffer), f);
    fclose(f);

    if (writeSize > 0) {
        return true;
    } else {
        this->sendErr("Did not write to spiffs");
        return false;
    }
}

bool OTAhandler::writeFirmware(const char* firmwareURL) {
    return true;
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