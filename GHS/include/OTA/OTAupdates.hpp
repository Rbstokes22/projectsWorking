#ifndef OTAUPDATES_HPP
#define OTAUPDATES_HPP

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"

namespace OTA {

// Used for development purposes only.
#define LOCAL_SERVER "http://192.168.86.246:5555/"

class OTAhandler {
    private:
    Comms::NetMain &station;
    static UI::Display* OLED;
    Messaging::MsgLogHandler &msglogerr;
    bool isConnected();
    
    // LAN UPDATES
    esp_ota_handle_t LANhandle;

    // WEB UPDATES
    esp_https_ota_handle_t WEBhandle;
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
    void update(const char* firmwareURL);
    void sendErr(const char* err);

    // LAN UPDATES
    void writeOTA(esp_http_client_handle_t client, int contentLen);
    void updateLAN();
    bool checkLAN();

    // WEB UPDATES
    void updateWEB(const char* firmwareURL);

};


}

#endif // OTAUPDATES_HPP