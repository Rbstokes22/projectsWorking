#include "OTA/OTAupdates.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "esp_http_client.h"
#include "Threads/Threads.hpp"
#include <cstddef>
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "FirmwareVal/firmwareVal.hpp"
#include "esp_crt_bundle.h"
#include "esp_transport.h"
#include "Config/config.hpp"
#include "Common/FlagReg.hpp"

namespace OTA {

URL::URL() {
    memset(this->firmware, 0, sizeof(URLSIZE));
    memset(this->signature, 0, sizeof(URLSIZE));
}

// Ensures that the net is actively connected to station mode.
// Returns true or false
bool OTAhandler::isConnected() {
    Comms::NetMode netMode = Comms::NetMain::getNetType();
    bool netConnected = station.isActive();

    return (netMode == Comms::NetMode::STA && netConnected);
}

// Opens all connections. Fetches header with an int64_t content
// length, and if > 0, returns the content length.
int64_t OTAhandler::openConnection() {
    if (!this->OLED.getOverrideStat()) { // Allows OLED population during OTA
        this->OLED.setOverrideStat(true);
    }

    if (DEVmode) { // used for development mode only.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    // Opens client connection, writes 0 meaning that no explicit write
    // function has to be called.
    if (esp_http_client_open(this->client, 0) != ESP_OK) {
        this->sendErr("Unable to open connection");
        this->close();
        return 0;
    } else {
        this->flags.setFlag(OTAFLAGS::CON);
    }

    int64_t contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        this->sendErr("Invalid content length");
        this->close();
        return 0;
    }

    return contentLength;
}

// Closes the appropriate connections that were previously opened. 
// Checks the flag to see if open, if so, attempts a close and 
// changes flag back to false. The checks are in linear order.
// Returns true or false.
bool OTAhandler::close() {
    uint8_t closeErrors{0};

    if (this->flags.getFlag(OTAFLAGS::OTA)) {

        if (esp_ota_end(this->OTAhandle) != ESP_OK) {
            this->sendErr("Unable to close OTA");
            closeErrors++;

        } else {
            this->flags.releaseFlag(OTAFLAGS::OTA);
        }
    }

    if (this->flags.getFlag(OTAFLAGS::CON) && !this->flags.getFlag(OTAFLAGS::OTA)) {
        if (esp_http_client_close(this->client) != ESP_OK) {
            this->sendErr("Unable to close client");
            closeErrors++;
        } else {
            this->flags.releaseFlag(OTAFLAGS::CON);
        }
    }

    if (this->flags.getFlag(INIT) && !this->flags.getFlag(CON)) {
        if (esp_http_client_cleanup(this->client) != ESP_OK) {
            this->sendErr("Unable to cleanup client");
            closeErrors++;
        } else {
            this->flags.releaseFlag(INIT);
        }
    }

    // Allows typical OLEd function, once closed.
    if (this->OLED.getOverrideStat()) {
        this->OLED.setOverrideStat(false);
    }
    
    return (closeErrors == 0);
}

// Processes the requests of the URL struct sent. Will receive both a firmware
// and signature URL. Inits and opens connection with the webserver, and passes
// the content length to writeSignature and writeFirmware methods. If both sig
// and firmware are successful, and firmware signature validation is successful,
// returns REQ_OK, else returns REQ_FAIL.
OTA_RET OTAhandler::processReq(URL &url) {

    const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);
    int64_t contentLen{0};
    size_t FW_SIZE{0};
    size_t SIG_SIZE{0};
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
    this->flags.setFlag(INIT);
    contentLen = this->openConnection();
    SIG_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    // Calls to write the signature. Closes 
    if (writeSignature(url.signature, label) == OTA_RET::SIG_FAIL) {
        this->close();
        this->sendErr("Unable to write signature");
        return OTA_RET::REQ_FAIL;
    }

    this->close();

    // config, init, and open http client
    this->config.url = url.firmware;
    this->client = esp_http_client_init(&this->config);
    this->flags.setFlag(INIT);
    contentLen = this->openConnection();
    FW_SIZE = (size_t)contentLen;

    if (contentLen <= 0) return OTA_RET::REQ_FAIL;

    OTA_RET FW = writeFirmware(url.firmware, part, contentLen);
   
    if (FW == OTA_RET::FW_FAIL) {
        this->sendErr("Unable to write firmware");
        this->close();
        return OTA_RET::REQ_FAIL;
    }

    this->close();

    // Validate partition. If good, sets the next boot partition.
    Boot::val_ret_t err = Boot::FWVal::get()->checkPartition(Boot::PART::NEXT, 
        FW_SIZE, SIG_SIZE);

    if (err != Boot::val_ret_t::VALID) {
        this->sendErr("OTA firmware invalid, will not set next partition");
        this->OLED.printUpdates("Firmware Sig Invalid");
        return OTA_RET::REQ_FAIL;
    }

    if (esp_ota_set_boot_partition(part) == ESP_OK) {
        this->OLED.printUpdates("OTA Success, restarting");
        printf("Signature valid, setting next partition\n");
        return OTA_RET::REQ_OK; 

    } else {
        this->OLED.printUpdates("OTA Fail");
        return OTA_RET::REQ_FAIL;
    }
}

// Accepts the sigURL and partititon label. Writes the signature to 
// spiffs. There are two partitions app0 and app1, and there are two 
// corresponding signatures app0firmware.sig and app1firmware.sig, which
// alternate. Returns SIG_OK or SIG_FAIL.
OTA_RET OTAhandler::writeSignature(const char* sigURL, const char* label) {
    size_t writeSize{0};
    char filepath[35]{0};
    uint8_t buffer[275]{0}; // Used to read the signature shouldnt exceed 260

    sprintf(filepath, "/spiffs/%sfirmware.sig", label); // either app0 or app1
    
    // C-casts buffer to char* since it is required by the read function.
    int dataRead = esp_http_client_read(this->client, (char*)buffer, sizeof(buffer));

    if (dataRead <= 0) {
        this->sendErr("Error reading signature Data");
        return OTA_RET::SIG_FAIL;
    }

    // Open the file to write in binary.
    FILE* f = fopen(filepath, "wb");
    if (f == NULL) {
        printf("Unable to open file: %s\n", filepath);
    }

    printf("Writing to filepath: %s\n", filepath);
    this->OLED.printUpdates("Writing Signature");

    // Writes the buffer into the appropriate spiffs file.
    writeSize = fwrite(buffer, 1, dataRead, f);
    fclose(f);
 
    if (writeSize > 0) {
        this->OLED.printUpdates("Signature OK");
        return OTA_RET::SIG_OK;
    } else {
        this->OLED.printUpdates("Signature Fail");
        this->sendErr("Did not write to spiffs");
        return OTA_RET::SIG_FAIL;
    }
}

// Accepts the firmware URL, partition, and content length. Writes the 
// stream of requested binary firmware data, via ota, to the next 
// partition in chunks. Returns FW_FAIL, FW_OTA_START_FAIL, and 
// FW_OK.
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

    // Begins the OTA with the appropriate content length.
    err = esp_ota_begin(part, contentLen, &this->OTAhandle);

    if (err != ESP_OK) {
        this->sendErr("OTA did not begin");
        return OTA_RET::FW_FAIL;
    } else {
        this->flags.setFlag(OTA);
    }

    if (part == NULL) {
        this->sendErr("No OTA partitition found");
        return OTA_RET::FW_FAIL;
    }

    // Checks for an active station connection.
    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return OTA_RET::FW_FAIL;
    }

    this->OLED.printUpdates("OTA writing firmware");
    printf("Writing firmware to label %s\n", part->label);

    // Loop to read data from the HTTP response in chunks, until complete.
    // dataRead is the primary control mechanism, and once there is no data
    // left to read, the loop breaks.
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

        snprintf( // Prints progress to OLED as float, until done.
            progress, 
            sizeof(progress), 
            " %.1f%%", 
            static_cast<float>(totalWritten) * 100 / contentLen);
        this->OLED.updateProgress(progress);
    }

    if (totalWritten == contentLen) {
        return OTA_RET::FW_OK;
    } else {
        return OTA_RET::FW_FAIL;
    }
}

// Requires message and level. Level default to INFO.
void OTAhandler::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg,
        Messaging::Method::SRL_LOG);
}

OTAhandler::OTAhandler(UI::Display &OLED, Comms::NetMain &station,
    Threads::Thread** toSuspend, size_t threadQty) : 

    flags("(OTAFlag)"), station(station), OLED(OLED), toSuspend(toSuspend), 
    threadQty(threadQty), OTAhandle(0), config{} {}
    

// Requires reference to firmware and signature url, and if the update is 
// LAN or WEB. This is the entry point for an update. Suspends all threads
// to be suspending during an update. Returns OTA_OK or OTA_FAIL.
OTA_RET OTAhandler::update(URL &url, bool isLAN) { 
    
    if (this->toSuspend == nullptr or *(this->toSuspend) == nullptr) {
        return OTA_RET::OTA_FAIL;
    }

    // Suspends all threads in the passed toSuspend thread array.
    for (int i = 0; i < this->threadQty; i++) {
        (*this->toSuspend[i]).suspendTask();
    }

    if (!isLAN) { // If web, set configuration settings.
        this->config.cert_pem = NULL;
        this->config.skip_cert_common_name_check = DEVmode; // false for testing
        this->config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    OTA_RET ret = this->processReq(url); // Process OTA update request.

    for (int i = 0; i < this->threadQty; i++) { // Lifts thread suspension.
        (*this->toSuspend[i]).resumeTask();
    }

    if (ret == OTA_RET::REQ_OK) {return OTA_RET::OTA_OK;}
    else {return OTA_RET::OTA_FAIL;}
}

// Allows the client to rollback to a previous version if there
// is a firmware issue.
bool OTAhandler::rollback() {
    const esp_partition_t* current = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    // Other partition
    const esp_partition_t* other = 
        (current->address != next->address) ? next : current;

    this->OLED.setOverrideStat(true); // Allow update display.

    if (esp_ota_set_boot_partition(other) == ESP_OK) {



        printf("Rolling back to partition %s\n", other->label);
        this->OLED.printUpdates("Rolling back to prev firmware"); // Blocking
        return true; // will prompt a restart, to release of override status.
    } else {
        this->OLED.printUpdates("Unable to Roll back"); // Blocking. Log as well.!!!!!
        this->OLED.setOverrideStat(false);
        return false;
    }
}

}