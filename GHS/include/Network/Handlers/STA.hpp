#ifndef STA_HPP
#define STA_HPP

#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"

namespace Comms {

void setOTAObject(OTA::OTAhandler &ota);
bool whitelistCheck(const char* URL); // add domains in the src file.

esp_err_t STAIndexHandler(httpd_req_t *req);
esp_err_t OTAUpdateHandler(httpd_req_t *req);
esp_err_t OTARollbackHandler(httpd_req_t *req);

}

#endif // STA_HPP