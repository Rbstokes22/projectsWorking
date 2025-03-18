#ifndef OTAUPDATES_HPP
#define OTAUPDATES_HPP

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"
#include "Threads/Threads.hpp"
#include "Common/FlagReg.hpp"
#include "UI/MsgLogHandler.hpp"
#include <cstddef>

namespace OTA {

#define URLSIZE 128 // Used for url query
#define OTA_CLEANUP_ATTEMPTS 5 // Attempts to try to close connection
#define OTA_BUFFER_SIZE 1024 // Used for signature and firmware.
#define OTA_FILEPATH_SIZE 32 // Used to access spiffs filepath
#define OTA_LOG_METHOD Messaging::Method::SRL_LOG

enum class OTA_RET { // Over the air update return values.
    OTA_OK, OTA_FAIL, LAN_CON, LAN_DISC,
    REQ_OK, REQ_FAIL, SIG_OK, SIG_FAIL, 
    FW_OK, FW_FAIL
};

enum OTAFLAGS : uint8_t {INIT, OPEN, OTA}; // Flags

struct URL {
    char firmware[URLSIZE]; // Firmware URL
    char signature[URLSIZE]; // Signature URL.
    URL();
};

class OTAhandler {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    uint8_t buffer[OTA_BUFFER_SIZE];
    Flag::FlagReg flags;  // OTA handling flags.
    Comms::NetMain &station; // station object, required or web OTA updates.
    UI::Display &OLED; // OLED used to update OTA progress, exclusive functions.
    Threads::Thread** toSuspend; // All threads to suspend for updates.
    size_t threadQty; // Total amount of threads.
    esp_ota_handle_t OTAhandle; // Handle for over the air updates.
    esp_http_client_config_t config; // http client configuration for web upd.
    esp_http_client_handle_t client; // http client handle for web upd.
    bool isConnected();
    bool initClient();
    bool openClient(int64_t &contentLen);
    bool cleanup(size_t attempt = 0);
    OTA_RET processReq(URL &url);
    bool processSig(URL &url, size_t &SIGSIZE, const esp_partition_t* part);
    bool processFW(URL &url, size_t &FWSIZE, const esp_partition_t* part);
    OTA_RET writeSignature(const char* sigURL, const char* label); 
    OTA_RET writeFirmware(const char* firmURL, const esp_partition_t* part,
        int64_t contentLen);

    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);
    
    public:
    OTAhandler(UI::Display &OLED,  Comms::NetMain &station,
        Threads::Thread** toSuspend, size_t threadQty);

    OTA_RET update(URL &url, bool isLAN = false);
    bool rollback();
};

}

#endif // OTAUPDATES_HPP