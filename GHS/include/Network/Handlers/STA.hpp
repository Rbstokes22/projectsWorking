#ifndef STA_HPP
#define STA_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"

namespace Comms {

esp_err_t STAIndexHandler(httpd_req_t* req);

// OTA specific, src file STAOTA.cpp

struct OTAupdates {
    cJSON* version;
    cJSON* signatureURL;
    cJSON* firmwareURL;
    cJSON* checksumURL;
};

void setOTAObject(OTA::OTAhandler &ota);
esp_err_t OTAUpdateHandler(httpd_req_t* req);
esp_err_t OTARollbackHandler(httpd_req_t* req);
esp_err_t OTACheckHandler(httpd_req_t* req);
cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size);
esp_err_t processJSON(cJSON* json, httpd_req_t* req, OTAupdates &otaData);
esp_err_t respondJSON(httpd_req_t* req, OTAupdates &otaData, const char* buffer);
bool whitelistCheck(const char* URL); // add domains in the src file.

}

#endif // STA_HPP