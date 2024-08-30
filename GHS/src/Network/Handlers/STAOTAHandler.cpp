#include "Network/Handlers/STAHandler.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "esp_http_client.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "esp_transport.h"


namespace Comms {

OTA::OTAhandler* OTA{nullptr};

void setOTAObject(OTA::OTAhandler &ota) {
    OTA = &ota;
}

void extractURL(httpd_req_t* req, char* URL, size_t size) {
    esp_err_t err;
    char response[30]{0};
    char query[128]{0};

    // Get the query string
    err = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (err != ESP_OK) {
        strcpy(response, "Failed to get query");
        httpd_resp_send(req, response, strlen(response));
    }

    // parse the value
    err = httpd_query_key_value(query, "url", URL, size);
    if (err != ESP_OK) {
        strcpy(response, "Failed to get URL");
        httpd_resp_send(req, response, strlen(response));
    }
    
    URL[size] = '\0';
}

esp_err_t OTAUpdateLANHandler(httpd_req_t* req) {
    char URL[128]{0};
    char response[30]{0};
    httpd_resp_set_type(req, "text/html");
    extractURL(req, URL, sizeof(URL));

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTA != nullptr && whitelistCheck(URL)) {
        if (OTA->update(URL, true) == true) {
            strcpy(response, "OK");
            httpd_resp_send(req, response, strlen(response));
        } else {
            strcpy(response, "Did not update OTA");
            httpd_resp_send(req, response, strlen(response));
        }
    }

    return ESP_OK;
}

// Used to download the OTA updates. INCLUDE SIGNATURE AND CHECKSUM AS WELL !!!!!!!!!!
esp_err_t OTAUpdateHandler(httpd_req_t* req) {
    char URL[128]{0};
    char response[30]{0};
    httpd_resp_set_type(req, "text/html");
    extractURL(req, URL, sizeof(URL));

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

// Checks if a new version of firmware is available.
// Fills buffer with json data, and returns 
// the parsed json object. Processes data to check firmware version.
// If version is different than current version, responds with the 
// json string from the https web server.
esp_err_t OTACheckHandler(httpd_req_t* req) {
    char buffer[350]{0};

    httpd_resp_set_type(req, "application/json");

    // returns parsed JSON string.
    cJSON* json = receiveJSON(req, buffer, sizeof(buffer));
    cJSON* version{NULL};
 
    // Processes the json to extract the version.
    if (processJSON(json, req, &version) != ESP_OK) {
        OTA->sendErr("Unable to process JSON");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    // Once processed, responds to server with buffer data if
    // firmware is new. If not, it will send a "match" response
    // which will not show an update on the webpage.
    esp_err_t resp = respondJSON(req, &version, buffer);
    cJSON_Delete(json);
    return resp;
}

// Sends request to https web server. This will copy the received JSON
// string into the passed buffer. Returns the parsed buffer as a 
// cJSON* pointer, or NULL.
cJSON* receiveJSON(httpd_req_t* req, char* buffer, size_t size) {

    // Use WEBURL from config.hpp, and concats it with the endpoint URL.
    char url[100]{0};
    strcpy(url, WEBURL);
    strcat(url, "/checkOTA");
    esp_err_t err;

    // Client configuration with the crt bundle attached for webserver 
    // cert validation. This is provided by esp and used mozillas data.
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .skip_cert_common_name_check = true, // CHANGE TO FALSE FOR PRODUCTION
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Cleans up the connection after receiving or attempting to receive data.
    auto cleanup = [&err, client](){
        err = esp_http_client_close(client);
        if (err != ESP_OK) {
            OTA->sendErr("Unable to close client if opened");
        }

        err = esp_http_client_cleanup(client);
        if (err != ESP_OK) {
            OTA->sendErr("Unable to cleanup client");
        }
    };

    // DELETE FOR PRODUCTION, USED IN NGROK ONLY TO SKIP PAGE.
    esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");

    // Opens connection in order to begin reading.
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        cleanup();
        OTA->sendErr("Unable to open connection with webserver");
        return NULL;
    }

    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        cleanup();
        OTA->sendErr("Invalid content length");
        return NULL;
    }

    // Filles the passed buffer with the JSON data read from web server.
    int dataRead = esp_http_client_read(client, buffer, size);
    buffer[dataRead] = '\0';

    cleanup();

    if (dataRead < 0) return NULL;
 
    return cJSON_Parse(buffer);
}

// Once data is read into the buffer, the parsed json pointer is passed
// along with the request and the version. Uses a double pointer, since 
// the version is a pointer to a pointer. Sets the version to point to 
// the version in the JSON string. Returns ESP_OK for valid JSON data,
// and ESP_FAIL for invalid.
esp_err_t processJSON(cJSON* json, httpd_req_t* req, cJSON** version) {
  
    if (json == NULL) {
     
        if (httpd_resp_sendstr(req, "{\"version\": \"Invalid JSON\"}") != ESP_OK) {
            OTA->sendErr("Error sending response");
        }

        return ESP_FAIL;
    }

    // Sets the version to point at the cJSON object.
    *version = cJSON_GetObjectItem(json, "version");

    // Checks that the version exists and is a string.
    if (*version != NULL && cJSON_IsString(*version)) {
        return ESP_OK;
    } else {
        if (httpd_resp_sendstr(req, "{\"version\": \"Invalid JSON\"}") != ESP_OK) {
            OTA->sendErr("Error sending response");
        }

        OTA->sendErr("Unable to get firmware version");
        return ESP_FAIL;
    }
}

// Following the JSON processing, the version and buffer are passed along with
// the request. Compares the version passed, to the current firmware version that 
// is kept in the config.hpp header. If they are equal, returns to the client, a
// version match. If they are not equal, sends entire response buffer from the 
// web server to the client for parsing, indicating an updatable version. If the
// data is corrups, sends a response of invalid JSON. Returns ESP_OK.
esp_err_t respondJSON(httpd_req_t* req, cJSON** version, const char* buffer) {

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.
    if (version != NULL && cJSON_IsString(*version)) {
        char _version[20]{0};
        strcpy(_version, (*version)->valuestring);

        if (strcmp(_version, FIRMWARE_VERSION) == 0) { // Match, no update
 
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
    
    return ESP_OK;
}

// Prevents malicious URLS from being used. Parses URL
// and returns true or false if accepted. This is used 
// for the firmware updates only, since the JSON OTAcheck
// is hardcoded into the program.
bool whitelistCheck(const char* URL) {

    // Add all acceptable domains
    const char* whitelistDomains[] {
        "http://192.168", // Local IP address
        "https://www.greenhouse.com", // Website
        "https://major-absolutely-bluejay.ngrok-free.app" // NGROK STATIC 
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