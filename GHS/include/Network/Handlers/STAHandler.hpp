#ifndef STAHANDLER_HPP
#define STAHANDLER_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"

namespace Comms {

// Src file STAHandler.cpp
esp_err_t STAIndexHandler(httpd_req_t* req);

// OTA specific, src file STAOTAHandler.cpp
void setOTAObject(OTA::OTAhandler &ota);
void extractURL(httpd_req_t* req, char* URL, size_t size);
esp_err_t OTAUpdateLANHandler(httpd_req_t* req);
esp_err_t OTAUpdateHandler(httpd_req_t* req);
esp_err_t OTARollbackHandler(httpd_req_t* req);

// OTA Check for updated version
esp_err_t OTACheckHandler(httpd_req_t* req);
cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size);
esp_err_t processJSON(cJSON* json, httpd_req_t* req, cJSON** version);
esp_err_t respondJSON(httpd_req_t* req, cJSON** verson, const char* buffer);

bool whitelistCheck(const char* URL); // add domains in the src file.

}

#endif // STAHANDLER_HPP