#ifndef OTAUPDATES_HPP
#define OTAUPDATES_HPP

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"

namespace OTA {

class OTAhandler {
    private:
    Comms::NetMain &station;
    static UI::Display* OLED;
    Messaging::MsgLogHandler &msglogerr;
    bool isConnected();
    esp_ota_handle_t OTAhandle;
    esp_http_client_config_t config;
    esp_http_client_handle_t client;
    
    // LAN UPDATES
    esp_ota_handle_t LANhandle; // DELETE

    // WEB UPDATES
    esp_https_ota_handle_t WEBhandle; // Delete
    static void event_handler(
        void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static size_t firmwareSize;
    void registerHandler();
    void unregisterHandler();
    
    public:
    OTAhandler(
        UI::Display &OLED, 
        Comms::NetMain &station,
        Messaging::MsgLogHandler &msglogerr);
    bool update(const char* firmwareURL, bool isLAN = false);
    bool checkLAN(const char* firmwareURL);
    void updateLAN(const char* firmwareURL);
    // Add a check WEB in as well
    bool updateWEB(const char* firmwareURL);
    bool writeSignature(const char* sigURL, const char* label); 
    bool writeFirmware(const char* firmwareURL);



    void sendErr(const char* err);
    void rollback();

    // LAN UPDATES
    
    bool writeOTA(esp_http_client_handle_t client, int contentLen);

    // WEB UPDATES
    

};


}

#endif // OTAUPDATES_HPP