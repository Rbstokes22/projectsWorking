#include "Network/Handlers/STAHandler.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

// Serves the station index page.
esp_err_t STAIndexHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, STApage, strlen(STApage));
    return ESP_OK;
}

// Serves the log entry.
esp_err_t STALogHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_sendstr(req, Messaging::MsgLogHandler::get()->getLog());
    return ESP_OK;
}

}