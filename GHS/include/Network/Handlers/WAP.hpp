#ifndef WAP_HPP
#define WAP_HPP

#include "esp_http_server.h"

namespace Comms {

esp_err_t WAPIndexHandler(httpd_req_t *req);

}

#endif // WAP_HPP