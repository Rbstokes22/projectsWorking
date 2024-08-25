#ifndef STA_HPP
#define STA_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"

namespace Comms {

struct OTAupdates {
    cJSON* version;
    cJSON* signatureURL;
    cJSON* firmwareURL;
};

void setOTAObject(OTA::OTAhandler &ota);
bool whitelistCheck(const char* URL); // add domains in the src file.
cJSON* receiveJSON(httpd_req_t* req);
esp_err_t processJSON(cJSON* json, httpd_req_t* req, OTAupdates &otaData);
esp_err_t responsdJSON(httpd_req_t* req, OTAupdates &otaData);

esp_err_t STAIndexHandler(httpd_req_t* req);
esp_err_t OTAUpdateHandler(httpd_req_t* req);
esp_err_t OTARollbackHandler(httpd_req_t* req);
esp_err_t checkOTAHandler(httpd_req_t* req);

}

#endif // STA_HPP