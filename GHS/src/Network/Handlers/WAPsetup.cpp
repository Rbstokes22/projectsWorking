#include "Network/Handlers/WAPsetup.hpp"
#include "Network/webPages.hpp"
#include "esp_http_server.h"

namespace Comms {

esp_err_t WAPSetupIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WAPSetupPage, strlen(WAPSetupPage));
    return ESP_OK;
}

esp_err_t WAPSubmitDataHandler(httpd_req_t *req) { // BUILD THIS USING JSON
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "TEMP", strlen("TEMP"));
    return ESP_OK;
}



}