#include "Network/Handlers/STA.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "cJSON.h"

namespace Comms {

OTA::OTAhandler* OTA{nullptr};

void setOTAObject(OTA::OTAhandler &ota) {
    OTA = &ota;
}

// Prevents malicious URLS from being used. Parses URL
// and returns true or false if accepted.
bool whitelistCheck(const char* URL) {

    // Add all acceptable domains (FOR testing add ngrok in here)
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

esp_err_t STAIndexHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, STApage, strlen(STApage));
    return ESP_OK;
}

// Handles all updates for the OTA.
esp_err_t OTAUpdateHandler(httpd_req_t* req) {
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

// This will be used to roll the OTA back to the previous version
esp_err_t OTARollbackHandler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "OK", strlen("OK"));

    if (OTA != nullptr) {
        OTA->rollback();
    }

    return ESP_OK;
}

// MOVE THESE
cJSON* receiveJSON(httpd_req_t* req) {
    char buffer[200]{0};
    int ret{0}, totalLength{0};

    while ((ret = httpd_req_recv(
        req, 
        buffer + totalLength, // next buffer address
        sizeof(buffer) - totalLength - 1)) > 0) {

        totalLength += ret;
    }

    if (ret < 0) { // indicates error
        OTA->sendErr("JSON request Error");
        return NULL;
    }

    buffer[totalLength] = '\0';
    return cJSON_Parse(buffer);
}

esp_err_t processJSON(cJSON* json, httpd_req_t* req, OTAupdates &otaData) {

    if (json == NULL) {

        if (httpd_resp_send_err(
            req, 
            HTTPD_400_BAD_REQUEST, 
            "Invalid JSON") != ESP_OK) {

            OTA->sendErr("Error sending response");
        }

        cJSON_Delete(json);
        cJSON_Delete(otaData.version);
        cJSON_Delete(otaData.firmwareURL);
        cJSON_Delete(otaData.signatureURL);
        return ESP_FAIL;
    }

    otaData.version = cJSON_GetObjectItem(json, "version");
    otaData.firmwareURL = cJSON_GetObjectItem(json, "firmwareURL");
    otaData.signatureURL = cJSON_GetObjectItem(json, "signatureURL");
    cJSON_Delete(json);

    return ESP_OK;
}

esp_err_t respondJSON(httpd_req_t* req, OTAupdates &otaData) {
    // delete the versions here from otaData

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.
    if (otaData.version != NULL && cJSON_IsString(otaData.version)) {
        const char* version = otaData.version->valuestring;

        // RESPOND WITH JSON

        if (strcmp(version, config::FIRMWARE_VERSION) == 0) {
            httpd_resp_send(req, "false", strlen("false"));
        } else {
            

        }
    }
}

// Check if a new version is available. If so, sends the firmware and 
// signature URL's to the webpage in JSON format.
esp_err_t checkOTAHandler(httpd_req_t* req) {
  
    httpd_resp_set_type(req, "application/json");

    cJSON* json = receiveJSON(req);

    OTAupdates otaData{nullptr, nullptr, nullptr};

    // Deletes the allocated json
    if (processJSON(json, req, otaData) != ESP_OK) {
        OTA->sendErr("Unable to set json objects");
        return ESP_FAIL;
    }

    return respondJSON(req, otaData);
}

}