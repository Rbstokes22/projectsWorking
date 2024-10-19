#ifndef WAPHANDLER_HPP
#define WAPHANDLER_HPP

#include "esp_http_server.h"

namespace Comms {

// No class necessary for single handler.
esp_err_t WAPIndexHandler(httpd_req_t *req);

}

#endif // WAPHANDLER_HPP