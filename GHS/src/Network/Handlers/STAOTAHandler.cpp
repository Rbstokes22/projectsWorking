#include "Network/Handlers/STAHandler.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_server.h"
#include "OTA/OTAupdates.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "esp_http_client.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "esp_transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

// Static Setup
const char* OTAHAND::tag("(OTAHAND)");
char OTAHAND::log[LOG_MAX_ENTRY]{0};
OTA::OTAhandler* OTAHAND::OTA{nullptr};
bool OTAHAND::isInit{false};

// Extracts url data. Passed request, url object, and size of each
// array. Parses query, and populate the url object firmware and 
// signature url. Returns true or false.
bool OTAHAND::extractURL(httpd_req_t* req, OTA::URL &urlOb, size_t size) {

    esp_err_t err;
    char query[URLSIZE * 2 + 50]{0}; // Leaves padding for query

    // Get the query string
    err = httpd_req_get_url_query_str(req, query, sizeof(query));
    if (err != ESP_OK) {
        httpd_resp_sendstr(req, "Failed to get query");
        return false;
    }

    // parse the value of the firmware url.
    err = httpd_query_key_value(query, "url", urlOb.firmware, size);
    urlOb.firmware[size] = '\0';

    if (err != ESP_OK && strlen(urlOb.firmware) > 0) {
        httpd_resp_sendstr(req, "Failed to get firmware URL");
        return false;
    }

    // parse the value of the signature url.
    err = httpd_query_key_value(query, "sigurl", urlOb.signature, size); 
    urlOb.signature[size] = '\0';

    if (err != ESP_OK && strlen(urlOb.signature) > 0) {
        httpd_resp_sendstr(req, "Failed to get signature URL");
        return false;
    }

    // Ensures the the firmware url is set. If > 0, it checks for a signature.
    // If the sig exists, returns the result of the whitelistChecks.
    if (strlen(urlOb.firmware) > 0) {
        if (strlen(urlOb.signature) > 0) {
            return whitelistCheck(urlOb.firmware) && 
                   whitelistCheck(urlOb.signature);
        } else {
            return whitelistCheck(urlOb.firmware);
        }
    }

    return false;
}

// Sends request to https web server. This will copy the received JSON
// string into the passed buffer. Returns the parsed buffer as a 
// cJSON* pointer, or NULL.
cJSON* OTAHAND::receiveJSON(httpd_req_t* req, char* buffer, size_t size) {

    // Use WEBURL from config.hpp, and concat it with the endpoint URL.
    char url[100]{0};
    strcpy(url, WEBURL);
    strcat(url, OTA_VERSION_PATH);
    esp_err_t err;

    // Client configuration with the crt bundle attached for webserver 
    // cert validation. This is provided by esp and used mozillas data.
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Cleans up the connection after receiving or attempting to receive data.
    auto cleanup = [&err, client](){
        err = esp_http_client_close(client);
        if (err != ESP_OK) {
            OTAHAND::OTA->sendErr("Unable to close client if opened");
        }

        err = esp_http_client_cleanup(client);
        if (err != ESP_OK) {
            OTAHAND::OTA->sendErr("Unable to cleanup client");
        }
    };

    if (DEVmode) { // used for development mode only.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    // Opens connection in order to begin reading.
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        cleanup();
        OTAHAND::OTA->sendErr("Unable to open connection with webserver");
        return NULL;
    }

    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        cleanup();
        OTAHAND::OTA->sendErr("Invalid content length");
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
esp_err_t OTAHAND::processJSON(cJSON* json, httpd_req_t* req, cJSON** version) {
  
    if (json == NULL) {
     
        if (httpd_resp_sendstr(req, "{\"version\":\"Invalid JSON\"}") != ESP_OK) {
            OTAHAND::OTA->sendErr("Error sending response");
        }

        return ESP_FAIL;
    }

    // Sets the version to point at the cJSON object.
    *version = cJSON_GetObjectItem(json, "version");

    // Checks that the version exists and is a string.
    if (*version != NULL && cJSON_IsString(*version)) {
        return ESP_OK;
    } else {
        if (httpd_resp_sendstr(req, "{\"version\":\"Invalid JSON\"}") != ESP_OK) {
            OTAHAND::OTA->sendErr("Error sending response");
        }

        OTAHAND::OTA->sendErr("Unable to get firmware version");
        return ESP_FAIL;
    }
}

// Following the JSON processing, the version and buffer are passed along with
// the request. Compares the version passed, to the cur firmware version that 
// is kept in the config.hpp header. If they are equal, returns to the client, a
// version match. If they are not equal, sends entire response buffer from the 
// web server to the client for parsing, indicating an updatable version. If the
// data is corrups, sends a response of invalid JSON. Returns ESP_OK.
esp_err_t OTAHAND::respondJSON(httpd_req_t* req, cJSON** version, 
    const char* buffer) {

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.
    if (version != NULL && cJSON_IsString(*version)) {
        char _version[20]{0};
        strcpy(_version, (*version)->valuestring);

        if (strcmp(_version, FIRMWARE_VERSION) == 0) { // Match, no update
 
            if (httpd_resp_sendstr(req, "{\"version\":\"match\"}") != ESP_OK) {
                OTAHAND::OTA->sendErr("Error sending response");
            } 

        } else { // No match, respond with request JSON from server.

            if (httpd_resp_sendstr(req, buffer) != ESP_OK) {
                OTAHAND::OTA->sendErr("Error sending response");
            } 
        }

    } else {
        if (httpd_resp_sendstr(req, "{\"version\": \"Invalid JSON\"}") != ESP_OK) {
            OTAHAND::OTA->sendErr("Error sending response");
        } 
    }
    
    return ESP_OK;
}

// Prevents malicious URLS from being used. Parses URL
// and returns true or false if accepted. This is used 
// for the firmware updates only, since the JSON OTAcheck
// is hardcoded into the program.
bool OTAHAND::whitelistCheck(const char* URL) {

    // Add all acceptable domains
    const char* whitelistDomains[] {
        "http://192.168", // Local IP address
        WEBURL // Website from config.hpp
    };

    size_t numDomains = sizeof(whitelistDomains) / sizeof(whitelistDomains[0]);

    for (size_t i = 0; i < numDomains; i++) {
        if (strstr(URL, whitelistDomains[i]) != NULL) {
            return true;
        }
    }

    return false;
}

// Requires message and messaging level. Level is default to ERROR. Prints
// messaging to log and serial.
void OTAHAND::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, 
        Messaging::Method::SRL_LOG);
}

bool OTAHAND::init(OTA::OTAhandler &ota) {
    OTAHAND::OTA = &ota;

    if (OTAHAND::OTA != nullptr) {
        OTAHAND::isInit = true;
    }

    return OTAHAND::isInit;
}

// Exclusive to OTA updates via LAN. The request is passed, and the 
// query string will contain only url, not sigurl, so only firmware
// url is used within the processing. Calls to update via OTA, and 
// returns ESP_OK regardless of integrity, since errors are handled
// via responses.
esp_err_t OTAHAND::updateLAN(httpd_req_t* req) {
    OTA::URL LANurl; // struct that holds both firmware and signature URLs.
  
    httpd_resp_set_type(req, "text/html");

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTAHAND::OTA != nullptr && extractURL(req, LANurl, URLSIZE)) {

        // Sends the url IP only. This version only produces a firmware URL.
        // Copes the fw url to the sig followed by a concat of the appropriate
        // endpoints.
        strcpy(LANurl.signature, LANurl.firmware);
        strcat(LANurl.firmware, LAN_OTA_FIRMWARE_PATH);
        strcat(LANurl.signature, LAN_OTA_SIG_PATH);

        if (OTAHAND::OTA->update(LANurl, true) == OTA::OTA_RET::OTA_OK) {

            // Prevents the client from waiting for a response and making a 
            // second request.
            if (httpd_resp_sendstr(req, "OK") == ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to allow response
                esp_restart();
            }

        } else {
            httpd_resp_sendstr(req, "OTA not updated");
        }
    } 

    httpd_resp_sendstr(req, "OTA not updated");

    return ESP_OK; // Must return ESP_OK
}

// Exclusive to OTA updates via web. The request is passed, and the 
// query string contains both url and sigurl. Calls to update via
// OTA, and returns ESP_OK regardless of integrity, since erros are 
// handled via resposnes.
esp_err_t OTAHAND::update(httpd_req_t* req) {
    OTA::URL WEBurl; // struct that holds both firmware and signature URLs.

    httpd_resp_set_type(req, "text/html");

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTAHAND::OTA != nullptr && extractURL(req, WEBurl, URLSIZE)) {

        if (OTAHAND::OTA->update(WEBurl, false) == OTA::OTA_RET::OTA_OK) {
            
            // Prevents the client from waiting for a response and making a 
            // second request.
            if (httpd_resp_sendstr(req, "OK") == ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to allow response
                esp_restart();
            }

        } else {
            httpd_resp_sendstr(req, "OTA FAIL");
        }
    } 

    return ESP_OK; // Must return ESP_OK
}

// This will be used to roll the OTA back to the previous version. Returns 
// ESP_OK after accepting request.
esp_err_t OTAHAND::rollback(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");

    if (OTAHAND::OTA != nullptr) {
        httpd_resp_sendstr(req, "OK");
        if (OTAHAND::OTA->rollback()) esp_restart();
    } else {
        httpd_resp_sendstr(req, "FAIL");
    }

    return ESP_OK; // Must return ESP_OK
}

// Checks if a new version of firmware is available.
// Fills buffer with json data, and returns 
// the parsed json object. Processes data to check firmware version.
// If version is different than current version, responds with the 
// json string from the https web server. Returns ESP_OK or ESP_FAIL.
esp_err_t OTAHAND::checkNew(httpd_req_t* req) {

    httpd_resp_set_type(req, "application/json");

    // Ensures that this is in station mode, to prevent checks while in WAP mode
    if (NetMain::getNetType() != NetMode::STA) {
        if (httpd_resp_sendstr(req, "{\"version\":\"wap\"}") != ESP_OK) {
            OTAHAND::OTA->sendErr("Error sending response");
            return ESP_OK; // Must return ESP_OK
        } 
    }

    char buffer[350]{0};

    // returns parsed JSON string.
    cJSON* json = receiveJSON(req, buffer, sizeof(buffer));
    cJSON* version{NULL};
 
    // Processes the json to extract the version.
    if (OTAHAND::processJSON(json, req, &version) != ESP_OK) {
        OTAHAND::OTA->sendErr("Unable to process JSON");
    }

    // Once processed, responds to server with buffer data if
    // firmware is new. If not, it will send a "match" response
    // which will not show an update on the webpage.
    OTAHAND::respondJSON(req, &version, buffer);
    cJSON_Delete(json);
    return ESP_OK; // Must return ESP_OK
}

}