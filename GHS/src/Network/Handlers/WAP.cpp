#include "Network/Handlers/WAP.hpp"
#include "Network/webPages.hpp"

namespace Comms {

esp_err_t WAPIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "WAP MAIN PAGE", strlen("WAP MAIN PAGE"));
    return ESP_OK;
}

}