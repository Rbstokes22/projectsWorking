#ifndef WAPSETUP_HPP
#define WAPSETUP_HPP

#include "esp_http_server.h"

namespace Comms {

esp_err_t WAPSetupIndexHandler(httpd_req_t *req);
esp_err_t WAPSubmitDataHandler(httpd_req_t *req);

}

#endif // WAPSETUP_HPP