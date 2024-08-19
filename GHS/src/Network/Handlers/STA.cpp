#include "Network/Handlers/STA.hpp"
#include "Network/webPages.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"

namespace Comms {

OTA::OTAhandler* OTA{nullptr};

void setOTAObject(OTA::OTAhandler &ota) {
    OTA = &ota;
}

esp_err_t STAIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, STApage, strlen(STApage));
    return ESP_OK;
}

esp_err_t OTAUpdateHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "OK", strlen("OK"));

    if (OTA != nullptr) {
        OTA->update("http://192.168.86.246:5555/OTAupdate"); // use query for production
    }

    return ESP_OK;
}

}