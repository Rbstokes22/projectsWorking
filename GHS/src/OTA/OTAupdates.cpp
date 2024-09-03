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
#include "esp_crt_bundle.h"
#include "esp_transport.h"

namespace OTA {
// Static setup

UI::Display* OTAhandler::OLED = nullptr;

URL::URL() {
    memset(this->firmware, 0, sizeof(URLSIZE));
    memset(this->signature, 0, sizeof(URLSIZE));
}

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

    // DELETE FOR PRODUCTION, USED IN NGROK ONLY TO SKIP PAGE
    esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");

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

OTA_RET OTAhandler::processReq(URL &url) {

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
    this->config.url = url.signature;
    this->client = esp_http_client_init(&this->config);
    contentLen = this->openConnection();
    FIRMWARE_SIG_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    if (writeSignature(url.signature, label) == OTA_RET::SIG_FAIL) {
        this->close(CLOSE_TYPE::CLOSE);
        this->sendErr("Unable to write signature");
        return OTA_RET::REQ_FAIL;
    }

    // config, init, and open http client
    this->config.url = url.firmware;
    this->client = esp_http_client_init(&this->config);
    contentLen = this->openConnection();
    FIRMWARE_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    OTA_RET FW = writeFirmware(url.firmware, part, contentLen);
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
        this->close(CLOSE_TYPE::OTA);
        this->sendErr("OTA firmware invalid, will not set next partition");
        OLED->printUpdates("Firmware Sig Invalid");
        return OTA_RET::REQ_FAIL;
    }

    if (esp_ota_set_boot_partition(part) == ESP_OK) {
        OLED->printUpdates("OTA Success, restarting");
        printf("Signature valid, setting next partition");
        this->close(CLOSE_TYPE::OTA);
        return OTA_RET::REQ_OK; 
    } else {
        OLED->printUpdates("OTA Fail");
        this->close(CLOSE_TYPE::OTA);
        return OTA_RET::REQ_FAIL;
    }
}

OTA_RET OTAhandler::writeSignature(const char* sigURL, const char* label) {
    size_t writeSize{0};
    char filepath[35]{0};
    uint8_t buffer[275]{0}; // Used to read the signature shouldnt exceed 260

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

    writeSize = fwrite(buffer, 1, dataRead, f);
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
    uint8_t buffer[1024]{0};
    int dataRead{0};
    int totalWritten{0};
    char progress[20]{0};

    err = esp_ota_begin(part, contentLen, &this->OTAhandle);
    if (err != ESP_OK) {
        this->sendErr("OTA did not begin");
        return OTA_RET::FW_OTA_START_FAIL;
    } 

    OLED->printUpdates("OTA writing firmware");
    printf("Writing firmware to label %s\n", part->label);

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
        dataRead = esp_http_client_read(client, (char*)buffer, sizeof(buffer));

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
    Messaging::MsgLogHandler &msglogerr,
    Threads::Thread** toSuspend,
    size_t threadQty) : 

    station(station), msglogerr(msglogerr), toSuspend(toSuspend), 
    threadQty(threadQty), OTAhandle(0) 
    
    {
        this->config = {0},
        OTAhandler::OLED = &OLED; 
    }

OTA_RET OTAhandler::update(
    URL &url, 
    bool isLAN
    ) { 

    auto Threads = [this](THREAD action){
        switch(action) {
            case THREAD::SUSPEND:
            for (int i = 0; i < this->threadQty; i++) {
                (*this->toSuspend[i]).suspendTask();
            }
            break;

            case THREAD::UNSUSPEND:
            for (int i = 0; i < this->threadQty; i++) {
                (*this->toSuspend[i]).resumeTask();
            }
            break;
        }
    };

    Threads(THREAD::SUSPEND);

    switch(isLAN) { 
        case true:
        if (this->processReq(url) == OTA_RET::REQ_OK) {
            Threads(THREAD::UNSUSPEND);
            return OTA_RET::OTA_OK;
        }
        break;

        case false: 
        this->config.cert_pem = NULL;
        this->config.skip_cert_common_name_check = true; // Change to false after testing
        this->config.crt_bundle_attach = esp_crt_bundle_attach;

        if (this->processReq(url) == OTA_RET::REQ_OK) {
            Threads(THREAD::UNSUSPEND);
            return OTA_RET::OTA_OK;
        }
        break;
    }

    Threads(THREAD::UNSUSPEND);
    return OTA_RET::OTA_FAIL;
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