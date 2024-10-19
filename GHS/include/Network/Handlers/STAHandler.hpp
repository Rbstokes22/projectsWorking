#ifndef STAHANDLER_HPP
#define STAHANDLER_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"

namespace Comms {

// Src file STAHandler.cpp
esp_err_t STAIndexHandler(httpd_req_t* req);

class OTAHAND {
    private:
    static OTA::OTAhandler* OTA;
    static bool extractURL(httpd_req_t* req, OTA::URL &urlOb, size_t size);
    static cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size);
    static esp_err_t processJSON(cJSON* json, httpd_req_t* req, cJSON** version);
    static esp_err_t respondJSON(httpd_req_t* req, cJSON** verson, const char* buffer);
    static bool whitelistCheck(const char* URL); // add domains in the src file.
    static bool isInit;

    public:
    static bool init(OTA::OTAhandler &ota);
    static esp_err_t updateLAN(httpd_req_t* req);
    static esp_err_t update(httpd_req_t* req);
    static esp_err_t rollback(httpd_req_t* req);
    static esp_err_t checkNew(httpd_req_t* req);
};

}

#endif // STAHANDLER_HPP