#include "Network/Handlers/STA.hpp"
#include "Network/webPages.hpp"
#include "esp_http_server.h"

namespace Comms {

esp_err_t STAIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "STA HOME PAGE", strlen("STA HOME PAGE"));
    return ESP_OK;
}

}