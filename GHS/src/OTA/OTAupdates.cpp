#include "OTA/OTAupdates.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "FirmwareVal/firmwareVal.hpp"
#include <cstddef>

namespace OTA {
// Static setup

UI::Display* OTAhandler::OLED = nullptr;

// Ensures that the net is actively connected to station mode.
// Returns true or false
bool OTAhandler::isConnected() {
    Comms::NetMode netMode = station.getNetType();
    bool netConnected = station.isActive();

    return (netMode == Comms::NetMode::STA && netConnected);
}

int64_t OTAhandler::openConnection() {
    if (!OLED->getOverrideStat()) {
        OLED->setOverrideStat(true);
    }

    if (esp_http_client_open(this->client, 0) != ESP_OK) {
        this->sendErr("Unable to open connection");
        this->close(CLOSE_TYPE::CLEAN);
        return 0;
    }

    int64_t contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        this->sendErr("Invalid content length");
        this->close(CLOSE_TYPE::CLOSE);
        return 0;
    }

    return contentLength;
}

bool OTAhandler::close(CLOSE_TYPE type) {
    uint8_t errors{0};

    switch(type) {
        case CLOSE_TYPE::CLEAN:
        if (esp_http_client_cleanup(this->client) != ESP_OK) {
            this->sendErr("Unable to cleanup client");
            errors++;
        }

        break;

        case CLOSE_TYPE::CLOSE:
        if (esp_http_client_close(this->client) != ESP_OK) {
            this->sendErr("Unable to close client");
            errors++;
        }

        if (esp_http_client_cleanup(this->client) != ESP_OK) {
            this->sendErr("Unable to cleanup client");
            errors++;
        }

        break;

        case CLOSE_TYPE::OTA:
        if (esp_http_client_close(this->client) != ESP_OK) {
            this->sendErr("Unable to close client");
            errors++;
        }

        if (esp_http_client_cleanup(this->client) != ESP_OK) {
            this->sendErr("Unable to cleanup client");
            errors++;
        }

        if (esp_ota_end(this->OTAhandle) != ESP_OK) {
            this->sendErr("Unable to close OTA");
            errors++;
        }

        break;
    }

    if (OLED->getOverrideStat()) {
        OLED->setOverrideStat(false);
    }
    
    return (errors == 0);
}

// Pings the index page of the local server to check if connected.
// Returns true of false depending on connection.
OTA_RET OTAhandler::checkLAN(const char* firmwareURL) {
    this->config.url = firmwareURL;
    this->client = esp_http_client_init(&this->config);

    if (this->openConnection() <= 0) {
        this->sendErr("Unable to open connection");
        this->close(CLOSE_TYPE::CLEAN);
        return OTA_RET::LAN_DISC;
    } else {
        this->close(CLOSE_TYPE::CLOSE);
        return OTA_RET::LAN_CON;
    }
}

OTA_RET OTAhandler::checkWEB(const char* firmwareURL) {
    return OTA_RET::WEB_CON;
}

OTA_RET OTAhandler::processReq(const char* sigURL, const char* firmURL) {

    const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);
    int64_t contentLen{0};
    size_t FIRMWARE_SIZE{0};
    size_t FIRMWARE_SIG_SIZE{0};
    char label[20]{0}; // Gets label name either app0 or app1
    strcpy(label, part->label);

    if (part == NULL) {
        this->sendErr("No OTA partitition found");
        return OTA_RET::REQ_FAIL;
    }

    // Checks for an active station connection.
    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return OTA_RET::REQ_FAIL;
    }

    // config, init, and open http client
    this->config.url = sigURL;
    this->client = esp_http_client_init(&this->config);
    contentLen = this->openConnection();
    FIRMWARE_SIG_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    if (writeSignature(sigURL, label) == OTA_RET::SIG_FAIL) {
        this->close(CLOSE_TYPE::CLOSE);
        this->sendErr("Unable to write signature");
        return OTA_RET::REQ_FAIL;
    }

    // config, init, and open http client
    this->config.url = firmURL;
    this->client = esp_http_client_init(&this->config);
    contentLen = this->openConnection();
    FIRMWARE_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    OTA_RET FW = writeFirmware(firmURL, part, contentLen);
    if (FW == OTA_RET::FW_OTA_START_FAIL) {
        this->sendErr("Unable to begin OTA");
        this->close(CLOSE_TYPE::CLOSE);
        return OTA_RET::REQ_FAIL;

    } else if (FW == OTA_RET::FW_FAIL) {
        this->sendErr("Unable to write firmware");
        this->close(CLOSE_TYPE::OTA);
        return OTA_RET::REQ_FAIL;
    }

    // Validate partition
    Boot::VAL err = Boot::checkPartition(
        Boot::PART::NEXT, 
        FIRMWARE_SIZE, 
        FIRMWARE_SIG_SIZE
        );

    if (err != Boot::VAL::VALID) {
        this->sendErr("OTA firmware invalid, will not set next partition");
        return OTA_RET::REQ_FAIL;
    }

    if (esp_ota_set_boot_partition(part) == ESP_OK) {
        OLED->printUpdates("OTA Success, restarting");
        this->close(CLOSE_TYPE::OTA);
        return OTA_RET::REQ_OK; // Doesnt matter, but exists for compiler reasons.
    } else {
        OLED->printUpdates("OTA Fail");
        this->close(CLOSE_TYPE::OTA);
        return OTA_RET::REQ_FAIL;
    }
}

OTA_RET OTAhandler::writeSignature(const char* sigURL, const char* label) {

    size_t writeSize{0};
    char filepath[35]{0};
    uint8_t buffer[265]{0}; // Used to read the signature shouldnt exceed 260

    sprintf(filepath, "/spiffs/%sfirmware.sig", label); // either app0 or app1
    
    int dataRead = esp_http_client_read(this->client, (char*)buffer, sizeof(buffer));

    if (dataRead <= 0) {
        this->sendErr("Error reading signature Data");
        return OTA_RET::SIG_FAIL;
    }

    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        printf("Unable to open file: %s\n", filepath);
    }

    printf("Writing to filepath: %s\n", filepath);
    OLED->printUpdates("Writing Signature");

    writeSize = fwrite(filepath, 1, sizeof(buffer), f);
    fclose(f);

    if (writeSize > 0) {
        OLED->printUpdates("Signature OK");
        return OTA_RET::SIG_OK;
    } else {
        OLED->printUpdates("Signature Fail");
        this->sendErr("Did not write to spiffs");
        return OTA_RET::SIG_FAIL;
    }
}

OTA_RET OTAhandler::writeFirmware(
    const char* firmURL,
    const esp_partition_t* part,
    int64_t contentLen
    ) {

    esp_err_t err;
    char buffer[1024]{0};
    int dataRead{0};
    int totalWritten{0};
    char progress[20]{0};

    err = esp_ota_begin(part, contentLen, &this->OTAhandle);
    if (err != ESP_OK) {
        this->sendErr("OTA did not begin");
        return OTA_RET::FW_OTA_START_FAIL;
    } 

    OLED->printUpdates("OTA writing firmware");

    if (part == NULL) {
        this->sendErr("No OTA partitition found");
        return OTA_RET::FW_FAIL;
    }

    // Checks for an active station connection.
    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return OTA_RET::FW_FAIL;
    }

    // Loop to read data from the HTTP response
    while (true) { 
        dataRead = esp_http_client_read(client, buffer, sizeof(buffer));

        if (dataRead < 0) {
            this->sendErr("http read error");
            return OTA_RET::FW_FAIL;
        }

        if (dataRead == 0) break;

        // Write the data to the OTA partition
        err = esp_ota_write(this->OTAhandle, buffer, dataRead);
        if (err != ESP_OK) {
            this->sendErr("OTA failed");
            return OTA_RET::FW_FAIL;
        }

        totalWritten += dataRead;

        snprintf(
            progress, 
            sizeof(progress), 
            " %.1f%%", 
            static_cast<float>(totalWritten) * 100 / contentLen);
        OLED->updateProgress(progress);
    }

    if (totalWritten == contentLen) {
        return OTA_RET::FW_OK;
    } else {
        return OTA_RET::FW_FAIL;
    }
}

OTAhandler::OTAhandler(
    UI::Display &OLED, 
    Comms::NetMain &station,
    Messaging::MsgLogHandler &msglogerr) :

    station(station), msglogerr(msglogerr), toSuspend(nullptr), 
    threadCt(0), OTAhandle(0)

    {
        OTAhandler::OLED = &OLED; 
    }

OTA_RET OTAhandler::update(const char* firmwareURL, bool isLAN) {
    char sigURL[100]{0};
    char firmURL[100]{0};

    auto Threads = [this](THREAD action){
        if (this->toSuspend == nullptr || this->threadCt == 0) {
            printf("Threads not passed to function or ct = 0\n");
            return;
        }
        switch(action) {
            case THREAD::SUSPEND:
            for (int i = 0; i < this->threadCt; i++) {
            this->toSuspend[i].suspendTask();
            }
            break;

            case THREAD::UNSUSPEND:
            for (int i = 0; i < this->threadCt; i++) {
            this->toSuspend[i].resumeTask();
            }
            break;
        }
    };

    printf("Handle %p\n", this->toSuspend[0].getHandle());
    Threads(THREAD::SUSPEND);
    printf("MOVED PAST SUSPEND\n"); return OTA_RET::OTA_FAIL; // Delete after testing!!!!!!!!!!

    switch(isLAN) { 
        case true:
        if (this->checkLAN(firmwareURL) == OTA_RET::LAN_CON) {
            sprintf(sigURL, "%s/%s", firmwareURL, "sigUpdate");
            sprintf(firmURL, "%s/%s", firmwareURL, "FWUpdate");

            if (this->processReq(sigURL, firmURL) == OTA_RET::REQ_OK) {
                Threads(THREAD::UNSUSPEND);
                return OTA_RET::OTA_OK;
            }
        }
        break;

        case false: // MIRROR ABOVE
        if (this->checkWEB(firmwareURL) == OTA_RET::WEB_CON) {
            // Mirror above and add in client->config for https

            if (this->processReq(sigURL, firmURL) == OTA_RET::REQ_OK) {
                Threads(THREAD::UNSUSPEND);
                return OTA_RET::OTA_OK;
            }
        }
        break;
    }

    Threads(THREAD::UNSUSPEND);
    return OTA_RET::OTA_FAIL;
}

void OTAhandler::passThreads(Threads::Thread* toSuspend, size_t threadCt) {
    this->toSuspend = toSuspend;
    this->threadCt = threadCt;
}

// Allows the client to rollback to a previous version if there
// is a firmware issue.
void OTAhandler::rollback() {
    const esp_partition_t* current = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    // Other partition
    const esp_partition_t* other = 
        (current->address == next->address) ? next : current;
    
    esp_ota_set_boot_partition(other);
    esp_restart();
}

// Primary error handler.
void OTAhandler::sendErr(const char* err) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR,
        err,
        Messaging::Method::SRL
        // Messaging::Method::OLED
    );
}

}