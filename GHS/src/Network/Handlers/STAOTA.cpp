#include "Network/Handlers/STA.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "esp_http_client.h"
#include "string.h"
#include "esp_crt_bundle.h"

namespace Comms {

OTA::OTAhandler* OTA{nullptr};

void setOTAObject(OTA::OTAhandler &ota) {
    OTA = &ota;
}

// Used to download the OTA updates. INCLUDE SIGNATURE AND CHECKSUM AS WELL !!!!!!!!!!
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

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTA != nullptr && whitelistCheck(URL)) {
        if (OTA->update(URL) == true) {
            strcpy(response, "OK");
            httpd_resp_send(req, response, strlen(response));
        } else {
            strcpy(response, "Did not update OTA");
            httpd_resp_send(req, response, strlen(response));
        }
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

// Check if a new version is available. If so, sends the firmware and 
// signature URL's to the webpage in JSON format.
esp_err_t OTACheckHandler(httpd_req_t* req) {
    char buffer[200]{0};

    httpd_resp_set_type(req, "application/json");

    cJSON* json = receiveJSON(req, buffer, sizeof(buffer));

    OTAupdates otaData{nullptr, nullptr, nullptr, nullptr};

    // Deletes the allocated json
    if (processJSON(json, req, otaData) != ESP_OK) {
        OTA->sendErr("Unable to set json objects");
        return ESP_FAIL;
    }

    return respondJSON(req, otaData, buffer);
}

// Notes
cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size) {

    char url[50]{0};
    strcpy(url, WEBURL);
    strcat(url, "/checkOTA");
    printf("URL: %s\n", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .skip_cert_common_name_check = false
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    printf("Client ERROR: %s\n", esp_err_to_name(err));

    // if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
    //     esp_http_client_cleanup(client);
    //     OTA->sendErr("Unable to open connection with webserver");
    //     return NULL;
    // }

    // int contentLength = esp_http_client_fetch_headers(client);

    // if (contentLength <= 0) {
    //     esp_http_client_cleanup(client);
    //     OTA->sendErr("Invalid content length");
    //     return NULL;
    // }

    int dataRead = esp_http_client_read(client, buffer, sizeof(buffer));

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (dataRead < 0) return NULL;

    buffer[dataRead] = '\0';
    return cJSON_Parse(buffer);

    // while ((ret = httpd_req_recv(
    //     req, 
    //     buffer + totalLength, // next buffer address
    //     size - totalLength - 1)) > 0) {

    //     totalLength += ret;
    // }

    // if (ret < 0) { // indicates error
    //     OTA->sendErr("JSON request Error");
    //     return NULL;
    // }

    // buffer[totalLength] = '\0';
    // return cJSON_Parse(buffer);
}

esp_err_t processJSON(cJSON* json, httpd_req_t* req, OTAupdates &otaData) {

    if (json == NULL) {
     
        if (httpd_resp_sendstr(req, "{\"version\": \"Invalid JSON\"}") != ESP_OK) {
            OTA->sendErr("Error sending response");
        }

        cJSON_Delete(json);
        cJSON_Delete(otaData.version);
        cJSON_Delete(otaData.firmwareURL);
        cJSON_Delete(otaData.signatureURL);
        cJSON_Delete(otaData.checksumURL);
        return ESP_FAIL;
    }

    otaData.version = cJSON_GetObjectItem(json, "version");
    otaData.firmwareURL = cJSON_GetObjectItem(json, "firmwareURL");
    otaData.signatureURL = cJSON_GetObjectItem(json, "signatureURL");
    otaData.checksumURL = cJSON_GetObjectItem(json, "checksumURL");
    cJSON_Delete(json);

    return ESP_OK;
}

esp_err_t respondJSON(httpd_req_t* req, OTAupdates &otaData, const char* buffer) {

    // delete the versions here from otaData !!!!!!!!!!!!!!!!!!

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.
    if (otaData.version != NULL && cJSON_IsString(otaData.version)) {
        const char* version = otaData.version->valuestring;

        if (strcmp(version, FIRMWARE_VERSION) == 0) { // Match, no update

            if (httpd_resp_sendstr(req, "{\"version\": \"match\"}") != ESP_OK) {
                OTA->sendErr("Error sending response");
            } 

        } else { // No match, respond with request JSON from server.
            
            if (httpd_resp_sendstr(req, buffer) != ESP_OK) {
                OTA->sendErr("Error sending response");
            } 
        }

    } else {
        if (httpd_resp_sendstr(req, "{\"version\": \"Invalid JSON\"}") != ESP_OK) {
            OTA->sendErr("Error sending response");
        } 
    }

    cJSON_Delete(otaData.version);
    cJSON_Delete(otaData.firmwareURL);
    cJSON_Delete(otaData.signatureURL);
    cJSON_Delete(otaData.checksumURL);

    return ESP_OK;
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

}