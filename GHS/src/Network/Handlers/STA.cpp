#include "Network/Handlers/STA.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "Network/webPages.hpp"

namespace Comms {

OTA::OTAhandler* OTA{nullptr};

void setOTAObject(OTA::OTAhandler &ota) {
    OTA = &ota;
}

// Prevents malicious URLS from being used. Parses URL
// and returns true or false if accepted.
bool whitelistCheck(const char* URL) {

    // Add all acceptable domains
    const char* whitelistDomains[] {
        "http://192.168", // Local IP address
        "https://www.greenhouse.com" // Website
    };

    size_t numDomains = sizeof(whitelistDomains) / sizeof(whitelistDomains[0]);

    for (size_t i = 0; i < numDomains; i++) {
        if (strstr(URL, whitelistDomains[i]) != NULL) {
            return true;
        }
    }

    return false;
}

esp_err_t STAIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, STApage, strlen(STApage));
    return ESP_OK;
}

// Handles all updates for the OTA.
esp_err_t OTAUpdateHandler(httpd_req_t *req) {
    esp_err_t err;
    char response[30]{0};
    char query[128]{0};
    char URL[128]{0};
    httpd_resp_set_type(req, "text/html");

    // Get the query string
    err = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (err != ESP_OK) {
        strcpy(response, "Failed to get query");
        httpd_resp_send(req, response, strlen(response));
        return ESP_FAIL;
    }

    // parse the value
    err = httpd_query_key_value(query, "url", URL, sizeof(URL));
    if (err != ESP_OK) {
        strcpy(response, "Failed to get URL");
        httpd_resp_send(req, response, strlen(response));
        return ESP_FAIL;
    }

    strcpy(response, "OK");
    httpd_resp_send(req, response, strlen(response));

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTA != nullptr && whitelistCheck(URL)) {
        OTA->update(URL); 
    }

    return ESP_OK;
}

esp_err_t OTARollbackHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "OK", strlen("OK"));

    if (OTA != nullptr) {
        OTA->rollback();
    }

    return ESP_OK;
}

}