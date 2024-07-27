#ifndef STA_HPP
#define STA_HPP

#include "esp_http_server.h"

namespace Comms {

esp_err_t STAIndexHandler(httpd_req_t *req);

}

#endif // STA_HPP