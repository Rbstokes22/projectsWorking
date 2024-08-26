#include "Network/Handlers/STA.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"

namespace Comms {

esp_err_t STAIndexHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, STApage, strlen(STApage));
    return ESP_OK;
}

}