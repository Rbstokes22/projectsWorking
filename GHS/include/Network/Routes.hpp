#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "Network/NetMain.hpp"
#include "Network/webPages.hpp"

namespace Communications {

esp_err_t WAPSetupIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WAPSetupPage, strlen(WAPSetupPage));
    return ESP_OK;
}

httpd_uri_t WAPSetupIndex = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = WAPSetupIndexHandler,
    .user_ctx  = NULL
};

}

#endif // ROUTES_HPP