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
#include "Peripherals/saveSettings.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Common/heartbeat.hpp"

namespace OTA {

// Holds the firmware and signature URLs.
URL::URL() {
    memset(this->firmware, 0, sizeof(URLSIZE));
    memset(this->signature, 0, sizeof(URLSIZE));
}

// Requires no params. Checks that the station connection is active and returns
// true or false.
bool OTAhandler::isConnected() {return station.isActive();}

// Requires no variables. Initializes the client based on the configuration.
// Returns true if successful, and false if not.
bool OTAhandler::initClient() {

    // If not init, then init. Sets client = NULL if error.
    if (!this->flags.getFlag(OTAFLAGS::INIT)) {
        this->client = esp_http_client_init(&this->config);
    }

    // Null check. If good set flag and return true.
    if (this->client != NULL) {
        this->flags.setFlag(OTAFLAGS::INIT);
        return true;

    } else { // Client not init. Log and return false.

        snprintf(this->log, sizeof(this->log), "%s Connection unable to init",
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }
}

// Requires no params. Opens client if it has been initiated. Returns true if
// successful, and false if not.
bool OTAhandler::openClient(int64_t &contentLen) {

    // WARNING. This function, when unresolved, will cause the heartbeat to
    // reset. This will extend all heartbeats by the timeout, in seconds to
    // prepare for a blocking function, only when needed, like this known case.
    // This is used exclusively here and will not effect any of the other
    // hearbeats.

    esp_err_t err = ESP_FAIL; // Defaul if below conditions are not met.

    if (DEVmode) { // used for development mode only. Set before opening.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    // Opens the connection if not already open, and it has been init.
    if (!this->flags.getFlag(OTAFLAGS::OPEN) && 
        this->flags.getFlag(OTAFLAGS::INIT)) {

        // As a prepatory measure, extend heartbeat times before opening client.
        heartbeat::Heartbeat* HB = heartbeat::Heartbeat::get();
        if (HB == nullptr) {
            snprintf(this->log, sizeof(this->log), "%s nullptr passed", 
                this->tag);

            this->sendErr(this->log);
            return false;
        }

        HB->suspendAll("OTA FW/SIG Client Request"); // Potential blocking prep.

        err = esp_http_client_open(this->client, 0); // just open, no write.

        HB->releaseAll(); // Release suspension to all.
    }

    if (err == ESP_OK) { // is open, set flag. Carry on
        this->flags.setFlag(OTAFLAGS::OPEN);

    } else { // Flag not set, unable to open. Cleanup.

        snprintf(this->log, sizeof(this->log), "%s Connection unable to open",
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    // Connection successfully opened.
    this->OLED.setOverrideStat(true); // Override to OLED print updates/prog.

    contentLen = esp_http_client_fetch_headers(client); // Set reference

    // Check content length
    if (contentLen < 0) { // Indicates error
        snprintf(this->log, sizeof(this->log), "%s invalid content len",
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    return true; // If content length is solid.
}

// Requires attempt number of cleanup. Defaults to 0. This has an internal
// handling system that will retry cleanups for failures, ultimately returning
// true if successful or false if not. Closes and cleans all connections
// iff their specific flag has been set to true during the process. WARNING:
// attempts should only be set within this function to retry another attempt. 
// All fresh cleanup calls should be 0.
bool OTAhandler::cleanup(size_t attempt) {
    attempt++; // Increment the passed attempt. This is to handle retries.

    if (attempt >= OTA_CLEANUP_ATTEMPTS) {
        this->OLED.setOverrideStat(false); // Release hold
        snprintf(this->log, sizeof(this->log), 
            "%s Unable to close connection, restarting", this->tag);

        this->sendErr(this->log, Messaging::Levels::CRITICAL);
        NVS::settingSaver::get()->save(); // Save peripheral settings

        esp_restart(); // Restart esp attempt.

        snprintf(this->log, sizeof(this->log), // Log if failure.
            "%s Unable to restart", this->tag);

        this->sendErr(this->log, Messaging::Levels::CRITICAL);
        return false; // Just in case it doesnt restart.
    }

    // Default to ESP_OK in event that any of the flags are not set.
    esp_err_t close{ESP_OK}, cleanup{ESP_OK}, otaEnd{ESP_OK};

    // First check OTA
    if (this->flags.getFlag(OTAFLAGS::OTA)) { // OTA has been opened
        otaEnd = esp_ota_end(this->OTAhandle);

        if (otaEnd != ESP_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s unable to close OTA att # %zu", this->tag, attempt);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            this->cleanup(attempt); // Retry.
        } 
        
        this->flags.releaseFlag(OTAFLAGS::OTA);
    }

    // Next check client open. OTA will have to be closed before invoking this.
    if (this->flags.getFlag(OTAFLAGS::OPEN)) {

        close = esp_http_client_close(this->client);

        if (close != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s unable to close client",
                this->tag);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            this->cleanup(attempt); // Retry.
        }

        this->flags.releaseFlag(OTAFLAGS::OPEN);
    }

    // Finally check init. Client will need to be closed before invoking this.
    if (this->flags.getFlag(OTAFLAGS::INIT)) {

        cleanup = esp_http_client_cleanup(this->client);

        if (cleanup != ESP_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s unable to cleanup client",this->tag);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            this->cleanup(attempt); // retry.
        }

        this->flags.releaseFlag(OTAFLAGS::INIT);
    }

    this->OLED.setOverrideStat(false); // Release hold.
    return (close == ESP_OK && cleanup == ESP_OK && otaEnd == ESP_OK);
}

// Requires the URL object reference with both the firmware and signature URL.
// Inits and opens connection with the hosting webserver. Processes the 
// request. If both the signature and firmware updates are successful, and
// the signature is validated, returns REQ_OK, else returns REQ_FAIL.
OTA_RET OTAhandler::processReq(URL &url) {

    const esp_partition_t* part = esp_ota_get_next_update_partition(NULL);

    if (part == NULL || part == nullptr) { // No partition.
        snprintf(this->log, sizeof(this->log), "%s no partition found", 
            this->tag);

        this->sendErr(this->log);
        return OTA_RET::REQ_FAIL;
    }

    if (!this->isConnected()) { // Checks for an active station connection.
        snprintf(this->log, sizeof(this->log), "%s not con to network", 
            this->tag);

        this->sendErr(this->log);
        return OTA_RET::REQ_FAIL;
    }

    // Partition exists and station is connected. Proceed.
    size_t FW_SIZE = 0; // Firmware size
    size_t SIG_SIZE = 0; // Signature size

    if (!this->processSig(url, SIG_SIZE, part)) return OTA_RET::REQ_FAIL;
    if (!this->processFW(url, FW_SIZE, part)) return OTA_RET::REQ_FAIL;

    // Successful write of both signature and firmware. Validate partition now.
    // If valid, set the next boot partition and trigger the http handler
    // to restart the device.

    // Validate partition. If good, sets the next boot partition.
    Boot::val_ret_t err = Boot::FWVal::get()->checkPartition(Boot::PART::NEXT, 
        FW_SIZE, SIG_SIZE);

    if (err != Boot::val_ret_t::VALID) {
        snprintf(this->log, sizeof(this->log), 
            "%s FW invalid, next part not set", this->tag); // !!!!!!!!!!!!!!!!!!! error

        this->sendErr(this->log, Messaging::Levels::WARNING);
        this->OLED.printUpdates("Firmware Sig Invalid"); // Blocking
        return OTA_RET::REQ_FAIL;
    }

    if (esp_ota_set_boot_partition(part) == ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s valid, set next partition", 
            this->tag);

        this->sendErr(this->log, Messaging::Levels::INFO);
        this->OLED.printUpdates("OTA Success, restarting"); // Blocking
        return OTA_RET::REQ_OK; 

    } else {
        snprintf(this->log, sizeof(this->log), "%s OTA Fail", this->tag);
        this->sendErr(this->log, Messaging::Levels::WARNING);
        this->OLED.printUpdates("OTA Fail"); // Blocking
        return OTA_RET::REQ_FAIL;
    }
}

// Requires url and signature size references, as well as the poiter to the
// partition. Processes the signature request by reading it from the client,
// and writing it to the ESP. Returns true if successful, or false if not.
bool OTAhandler::processSig(URL &url, size_t &SIGSIZE, 
    const esp_partition_t* part) {
    
    // Copy over either app0 or app1 to the partition label. Used to label 
    // appropriate signature to partition.
    char label[sizeof(part->label)]{0}; 
    snprintf(label, sizeof(label), "%s", part->label); 
    int64_t contentLen = 0;

    // Process the signature. Configure the client using config settings.
    this->config.url = url.signature;

    // Initialize client and open connection. Pass contentLength to be 
    // populated.
    if (!this->initClient()) return false;
    if (!this->openClient(contentLen)) return false;

    SIGSIZE = static_cast<size_t>(contentLen); // Convert
   
    // Calls to write the signature. Closes 
    if (writeSignature(url.signature, label) == OTA_RET::SIG_FAIL) { 
        this->cleanup();
        return false; // Error logging within callback.
    }

    return this->cleanup(); // Close and clean connection. If success ret true.
}

// Requires url and firmware size references, as well as the poiter to the
// partition. Processes the firmware request by reading it from the client,
// and writing it to the ESP. Returns true if successful, or false if not.
bool OTAhandler::processFW(URL &url, size_t &FWSIZE, 
    const esp_partition_t* part) {
    
    int64_t contentLen = 0;

    // Configure the client using config settings.
    this->config.url = url.firmware;

    // Initialize client and open connection. Pass content length to be pop.
    if (!this->initClient()) return false;
    if (!this->openClient(contentLen)) return false;

    FWSIZE = static_cast<size_t>(contentLen); // Convert

    if (writeFirmware(url.firmware, part, contentLen) == OTA_RET::FW_FAIL) {
        this->cleanup();
        return false;
    }

    return this->cleanup(); // Close and clean connection. If success ret true.
}

// Requires signature url and label. Writes the signauture to the spiffs using
// the passed label. There are two partitions app0 and app1, which have 
// corresponding signatures being app0firmware.sig and app1firmware.sig, 
// which alternate. Returns SIG_OK or SIG_FAIL.
OTA_RET OTAhandler::writeSignature(const char* sigURL, const char* label) {
    size_t writeSize{0}; // Total size written to file.
    char filepath[OTA_FILEPATH_SIZE]{0}; 

    // Adds either app0 or app1 to the filepath.
    snprintf(filepath, sizeof(filepath), "/spiffs/%sfirmware.sig", label);
    
    // C-casts buffer to char* since it is required by the read function.
    int dataRead = esp_http_client_read(this->client, (char*)this->buffer, 
        sizeof(this->buffer));

    if (dataRead <= 0) { // Indicates error if < 0. Captured a 0 read as well.
        snprintf(this->log, sizeof(this->log), 
            "%s error reading signature data", this->tag);

        this->sendErr(this->log);
        return OTA_RET::SIG_FAIL;
    }

    // Open the file to write in binary.
    FILE* f = fopen(filepath, "wb");

    if (f == NULL || f == nullptr) {
        snprintf(this->log, sizeof(this->log), 
            "%s unable to open filepath %s", this->tag, filepath);

        this->sendErr(this->log);
        return OTA_RET::SIG_FAIL;
    }

    snprintf(this->log, sizeof(this->log), "%s writing to filepath %s",
        this->tag, filepath);

    this->sendErr(this->log, Messaging::Levels::INFO);
    this->OLED.printUpdates("Writing Signature"); // Blocks before writing.

    // Writes the buffer into the appropriate spiffs file.
    writeSize = fwrite(this->buffer, 1, dataRead, f);
    fclose(f);
 
    if (writeSize > 0) {
        snprintf(this->log, sizeof(this->log), "%s signature OK", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
        this->OLED.printUpdates("Signature OK");
        return OTA_RET::SIG_OK;

    } else {
        snprintf(this->log, sizeof(this->log), "%s signature fail", this->tag);
        this->sendErr(this->log, Messaging::Levels::WARNING);
        this->OLED.printUpdates("Signature Fail");
        return OTA_RET::SIG_FAIL;
    }
}

// Accepts the firmware URL, partition, and content length. Writes the 
// stream of requested binary firmware data, via ota, to the next 
// partition in chunks. Returns FW_FAIL, FW_OTA_START_FAIL, and 
// FW_OK.
OTA_RET OTAhandler::writeFirmware(const char* firmURL,
    const esp_partition_t* part, int64_t contentLen) {

    esp_err_t err;
    memset(this->buffer, 0, sizeof(this->buffer));
    int dataRead{0};
    int totalWritten{0};
    char progress[20]{0};

    // Initialize and begin the ota if not began.
    if (!this->flags.getFlag(OTAFLAGS::OTA)) { // Safety check.

        // Begins the OTA with the appropriate content length.
        err = esp_ota_begin(part, contentLen, &this->OTAhandle);

        if (err != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s OTA did not begin", 
                this->tag);

            this->sendErr(this->log); // Cleanup handled in calling func.
            return OTA_RET::FW_FAIL;
        }

        this->flags.setFlag(OTAFLAGS::OTA);
    }

    // Check partition validity.
    if (part == NULL || part == nullptr) {
        snprintf(this->log, sizeof(this->log), "%s OTA pos not found", 
            this->tag);

        this->sendErr(this->log); // Cleanup handled in calling func.
        return OTA_RET::FW_FAIL;
    }

    // Checks are good, go ahead and begin writing firmware to esp.
    snprintf(this->log, sizeof(this->log), "%s OTA writing FW to label %s",
        this->tag, part->label);

    this->sendErr(this->log, Messaging::Levels::INFO);
    this->OLED.printUpdates("OTA writing firmware");

    // Loop to read data from the HTTP response in chunks, until complete.
    // dataRead is the primary control mechanism, and once there is no data
    // left to read, the loop breaks.
    while (true) { 

        dataRead = esp_http_client_read(this->client, (char*)this->buffer, 
            sizeof(this->buffer));

        if (dataRead < 0) { // Indicates error
            snprintf(this->log, sizeof(this->log), "%s http read err", 
                this->tag);

            this->sendErr(this->log);
            return OTA_RET::FW_FAIL;
        }

        if (dataRead == 0) break; // Nothing left to read/write

        // Write the data to the OTA partition
        err = esp_ota_write(this->OTAhandle, this->buffer, dataRead);

        if (err != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s OTA fail", this->tag);
            this->sendErr(this->log);
            return OTA_RET::FW_FAIL;
        }

        // If successful write, incremenet the total written by the data read.
        totalWritten += dataRead;

        // One of the few times the OLED update progress is used. This is 
        // non-blocking and updates the float value based on the current 
        // progress.
        snprintf(progress, sizeof(progress), " %.1f%%", 
            static_cast<float>(totalWritten) * 100 / contentLen);

        this->OLED.updateProgress(progress); // Write progress to OLED.
    }

    // Once complete, ensure the total written is equal to the total read.
    if (totalWritten == contentLen) {
        snprintf(this->log, sizeof(this->log), "%s write complete", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
        return OTA_RET::FW_OK;
    } else {
        snprintf(this->log, sizeof(this->log), "%s write fail", this->tag);
        this->sendErr(this->log, Messaging::Levels::WARNING);
        return OTA_RET::FW_FAIL;
    }
}

// Requires message and level. Level default to ERROR.
void OTAhandler::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, OTA_LOG_METHOD);
}

OTAhandler::OTAhandler(UI::Display &OLED, Comms::NetMain &station,
    Threads::Thread** toSuspend, size_t threadQty) : 

    tag(OTA_TAG),
    flags("(OTAUpdFlag)"), station(station), OLED(OLED), toSuspend(toSuspend), 
    threadQty(threadQty), OTAhandle(0), config{} {

    memset(this->buffer, 0, sizeof(this->buffer));
    }
    
// Requires reference to firmware and signature url, and if the update is 
// LAN or WEB. This is the entry point for an update. Suspends all threads
// to be suspending during an update. Returns OTA_OK or OTA_FAIL.
OTA_RET OTAhandler::update(URL &url, bool isLAN) { 
    
    if (this->toSuspend == nullptr or *(this->toSuspend) == nullptr) {
        return OTA_RET::OTA_FAIL; // Gate.
    }

    // Suspends all threads in the passed toSuspend thread array.
    for (int i = 0; i < this->threadQty; i++) {
        (*this->toSuspend[i]).suspendTask();
    }

    this->config.timeout_ms = WEB_TIMEOUT_MS; // Set config timeout upon update.

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
        snprintf(this->log, sizeof(this->log), 
            "%s rolling back to partition %s", this->tag, other->label);

        this->sendErr(this->log, Messaging::Levels::INFO);
        this->OLED.printUpdates("Rolling back to prev firmware"); // Blocking
        this->OLED.setOverrideStat(false);
        return true; // will prompt a restart with handler.

    } else { // Unable to roll back.

        snprintf(this->log, sizeof(this->log), 
            "%s Unable to roll back partition", this->tag);

        this->sendErr(this->log, Messaging::Levels::WARNING);
        this->OLED.printUpdates("Unable to Roll back"); // Blocking. 
        this->OLED.setOverrideStat(false);
        return false;
    }
}

}