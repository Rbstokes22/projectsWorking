#ifndef STAHANDLER_HPP
#define STAHANDLER_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/MasterHandler.hpp"

namespace Comms {

#define OTA_URL_SIZE 100 // max size of url
#define OTA_CHECKNEW_BUFFER_SIZE 300 // json data with some padding, est 200.

// Src file STAHandler.cpp
esp_err_t STAIndexHandler(httpd_req_t* req);
esp_err_t STALogHandler(httpd_req_t* req);

// Indicies use for close and cleanup flags.
enum OTAFLAGS : uint8_t {OPEN, INIT};

// NOTE: All static variables and functions due to requirements of the URI
// handlers. Could be a singleton, but didnt want to rewrite. 

class OTAHAND : public MASTERHAND {
    private:
    static const char* tag;
    static bool isInit; // Is handler init
    static OTA::OTAhandler* OTA; // OTA pointer, passed ref in init func.
    static bool extractURL(httpd_req_t* req, OTA::URL &urlOb, size_t size);
    static cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size);
    static bool processJSON(cJSON* json, httpd_req_t* req, 
        cJSON** version);

    static bool respondJSON(httpd_req_t* req, cJSON** verson, 
        const char* buffer);
        
    static bool whitelistCheck(const char* URL); // add domains in the src file.
    static bool initClient(esp_http_client_handle_t &client, 
        esp_http_client_config_t &config, Flag::FlagReg &flag,
        httpd_req_t* req);

    static bool openClient(esp_http_client_handle_t &client, 
        Flag::FlagReg &flag, httpd_req_t* req);

    static bool initCheck(httpd_req_t* req);
    static bool cleanup(esp_http_client_handle_t &client, Flag::FlagReg &flag, 
        size_t attempt = 0);

    public:
    static bool init(OTA::OTAhandler &ota);
    static esp_err_t updateLAN(httpd_req_t* req);
    static esp_err_t update(httpd_req_t* req);
    static esp_err_t rollback(httpd_req_t* req);
    static esp_err_t checkNew(httpd_req_t* req);
};

}

#endif // STAHANDLER_HPP