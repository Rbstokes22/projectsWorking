#ifndef OTAUPDATES_HPP
#define OTAUPDATES_HPP

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"
#include "Threads/Threads.hpp"
#include <cstddef>

namespace OTA {

#define URLSIZE 128 // Used for url query

enum class OTA_RET {
    OTA_OK, OTA_FAIL, LAN_CON, LAN_DISC,
    REQ_OK, REQ_FAIL, SIG_OK, SIG_FAIL, 
    FW_OK, FW_FAIL
};

enum class THREAD {
    SUSPEND, UNSUSPEND
};

struct URL {
    char firmware[URLSIZE];
    char signature[URLSIZE];
    URL();
};

struct CloseFlags {
    bool init;
    bool conn;
    bool ota;
};

class OTAhandler {
    private:
    static CloseFlags flags;
    Comms::NetMain &station;
    static UI::Display* OLED;
    Messaging::MsgLogHandler &msglogerr;
    Threads::Thread** toSuspend;
    size_t threadQty;
    esp_ota_handle_t OTAhandle;
    esp_http_client_config_t config;
    esp_http_client_handle_t client;
    bool isConnected();
    int64_t openConnection();
    bool close();
    OTA_RET processReq(URL &url);
    OTA_RET writeSignature(const char* sigURL, const char* label); 
    OTA_RET writeFirmware(
        const char* firmURL, 
        const esp_partition_t* part,
        int64_t contentLen
        );
    
    public:
    OTAhandler(
        UI::Display &OLED, 
        Comms::NetMain &station,
        Messaging::MsgLogHandler &msglogerr,
        Threads::Thread** toSuspend,
        size_t threadQty
        );

    OTA_RET update(
        URL &url, 
        bool isLAN = false
        );

    bool rollback();
    void sendErr(const char* err);
    
};

}

#endif // OTAUPDATES_HPP