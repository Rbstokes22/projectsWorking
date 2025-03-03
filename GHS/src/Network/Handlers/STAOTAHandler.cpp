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
#include "Network/Handlers/MasterHandler.hpp"

namespace Comms {

// Static Setup
const char* OTAHAND::tag("(OTAHAND)");
bool OTAHAND::isInit{false};
OTA::OTAhandler* OTAHAND::OTA{nullptr};

// Requires request pointer, ref to URL object, and the size of the URL.
// Parses query, and populates the url object firmware and signature URL. 
// Returns true if successful, and false if not. Will also respond to the 
// client a failure message if unsuccessful.
bool OTAHAND::extractURL(httpd_req_t* req, OTA::URL &urlOb, size_t size) {

    esp_err_t err;

    char query[URLSIZE * 2 + 50]{0}; // +50 leaves padding for query

    // Get the query string
    err = httpd_req_get_url_query_str(req, query, sizeof(query));

    if (err != ESP_OK) { // If error getting query, send web response.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Query Fail", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "extrurl");
        
        return false;
    }

    // parse the value of the firmware url by getting query key "url"
    err = httpd_query_key_value(query, "url", urlOb.firmware, size);

    urlOb.firmware[size] = '\0'; // Ensure null term despite memset

    // Checks the key was found and there is a length. If not, returns false.
    if (err != ESP_OK || strlen(urlOb.firmware) <= 0) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Failed to get URL", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "extrurl");

        return false;
    }

    // parse the value of the signature url by getting query key "sigurl"
    err = httpd_query_key_value(query, "sigurl", urlOb.signature, size); 
    urlOb.signature[size] = '\0'; // null term despite memset.

    // Checks the key was found and there is a length. If not, returns false.
    if (err != ESP_OK || strlen(urlOb.signature) <= 0) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s SigURL acquire FAIL", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "extrurl");

        return false;
    }

    // Ensures the the firmware url is set. If > 0, it checks for a signature.
    // If the sig exists, returns the result of the whitelistChecks to ensure
    // the url is permitted.
    if (strlen(urlOb.firmware) > 0 && strlen(urlOb.signature) > 0) {
        // INDICATES WEB OTA UPDATE
        return whitelistCheck(urlOb.firmware) && 
            whitelistCheck(urlOb.signature); // true if both pass check.

    } else if (strlen(urlOb.firmware) > 0) {
        // INDICATES LAN UPDATE SINCE SIGNATURE IS NOT PASSED.
        return whitelistCheck(urlOb.firmware);
    }

    return false; // Data bad.
}

// Requires the http request, buffer to be written to, and the buffer size.
// Sends the request to the web server. This will copy the received JSON
// into the passed buffer. Returns the parsed buffer as a cJSON* pointer,
// or NULL if error. ATTENTION!!! This function is exclusive to checking for
// OTA updates and not the update itself, which is managed in this src file,
// but occurs in the OTA class. 
cJSON* OTAHAND::receiveJSON(httpd_req_t* req, char* buffer, size_t size) {

    // ATTENTION: No response string for bad data, if this function is 
    // unsuccessful, the response string will be sent in processJSON.

    // Use WEBURL from config.hpp, and concat it with the endpoint URL.
    char url[OTA_URL_SIZE]{0};
    strcpy(url, WEBURL);
    strcat(url, OTA_VERSION_PATH);
    esp_err_t err;

    // Client configuration with the crt bundle attached for webserver 
    // cert validation. This is provided by esp and uses mozillas data.
    // When DEVmode changes to false, it will be handled by the server.
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    // Create client using configuration.
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Cleans up the connection after receiving or attempting to receive data.
    // If close, will attempt to close if opened. Defaulted to true, but passing
    // false is acceptable for a connection that has not been opened.
    auto cleanup = [&err, client](bool close = true){

        if (close) { // If never opened, you can bypass.
            err = esp_http_client_close(client); // Attempt client close.

            if (err != ESP_OK) { 
                
                snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                    "%s Unable to close client. %s", OTAHAND::tag,
                    esp_err_to_name(err));

                MASTERHAND::sendErr(MASTERHAND::log);
            }
        }
        
        err = esp_http_client_cleanup(client); // Attempts to cleanup client.

        if (err != ESP_OK) {

            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                "%s Unable to cleanup client. %s", OTAHAND::tag, 
                esp_err_to_name(err));

            MASTERHAND::sendErr(MASTERHAND::log);
        }
    };

    if (DEVmode) { // used for development mode only, req by ngrok.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    err = esp_http_client_open(client, 0); // Open connection, no write.

    // Opens connection in order to begin reading. 
    if (err != ESP_OK) {
        cleanup(false); // Never opened, do not need to close.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Unable to open con, %s", OTAHAND::tag, esp_err_to_name(err));

        MASTERHAND::sendErr(MASTERHAND::log);

        return NULL; // return NULL if error.
    }

    // Get the payload length through the headers.
    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) { // If no length, cleanup.
        cleanup();

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Invalid content length of %d. %s", OTAHAND::tag, contentLength,
            esp_err_to_name(err));

        MASTERHAND::sendErr(MASTERHAND::log);
        return NULL; // return NULL if error.
    }

    // Fills the passed buffer with the JSON data read from web server.
    int dataRead = esp_http_client_read(client, buffer, size);
    buffer[dataRead] = '\0';

    cleanup(); // Run a final cleanup if good.

    if (dataRead < 0) return NULL;
 
    return cJSON_Parse(buffer); // Return parsed buffer if data is solid.
}

// Once data is read into the buffer, the parsed json pointer is passed
// along with the request and the version. Uses a double pointer, since 
// the version is a pointer to a pointer. Sets the version to point to 
// the version in the JSON string. Returns ESP_OK for valid JSON data,
// and ESP_FAIL for invalid.
esp_err_t OTAHAND::processJSON(cJSON* json, httpd_req_t* req, cJSON** version) {
    
    if (json == NULL || json == nullptr) { // if null, log and resp to client

        // Handles processing bad JSON if NULL is returned in receiveJSON().
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s JSON process fail", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
            "{\"version\":\"Invalid JSON\"}"), OTAHAND::tag, "procjson");

        return ESP_FAIL; // Block remaining code.
    }

    // Sets the version to point at the cJSON object with key = version.
    *version = cJSON_GetObjectItem(json, "version");

    // Checks that the version exists and is a string. Returns OK if good,
    // if not good, logs and responds to client.
    if (*version != NULL && cJSON_IsString(*version)) {
        return ESP_OK;

    } else { // Version does not exist or is not a string.

        *version = NULL; // Ensure version set back to NULL.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Unable to get firmware version", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
            "{\"version\":\"Invalid JSON\"}"), OTAHAND::tag, "procjson");

        return ESP_FAIL;
    }
}

// Following the JSON processing, the version and buffer are passed along with
// the request. Compares the version passed, to the cur firmware version that 
// is kept in the config.hpp header. If they are equal, returns to the client, a
// version match. If they are not equal, sends entire response buffer from the 
// web server to the client for parsing, indicating an updatable version. If the
// data is corrupt, sends a response of invalid JSON. Returns ESP_OK.
esp_err_t OTAHAND::respondJSON(httpd_req_t* req, cJSON** version, 
    const char* buffer) {

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.
    if (version != NULL && cJSON_IsString(*version)) {
        char _version[20]{0};
        strcpy(_version, (*version)->valuestring);

        if (strcmp(_version, FIRMWARE_VERSION) == 0) { // Match, no update
        
            // No log required. Only response string.
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
                "{\"version\":\"match\"}"), OTAHAND::tag, "respjson");

        // No match, respond with request JSON from server indicating to
        // client there is a version for downloading. Sends data.
        } else { 

            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, buffer), 
                OTAHAND::tag, "procjson");
        }

    } else { // Bad data passed.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Bad JSON response data", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
            "{\"version\":\"Invalid JSON\"}"), OTAHAND::tag, "respjson");
    }
    
    return ESP_OK;
}

// Requires URL to check against. Parses the whitelist domains and if the 
// required text is within the URL, returns true. If no match, returns false.
// ATTENTION: Used in firmware updates only, since the JSON OTAcheck is 
// hardcoded into the program.
bool OTAHAND::whitelistCheck(const char* URL) {

    // White list domains on config source and header.

    size_t numDomains = sizeof(whiteListDomains) / sizeof(whiteListDomains[0]);

    // Iterate the domains in the whitelist domains, and return true if the
    // URL equals a portion of the allowed domains. If not, returns false.
    for (size_t i = 0; i < numDomains; i++) {
        if (strstr(URL, whiteListDomains[i]) != NULL) { // Match
            return true;
        }
    }

    return false;
}

// Requires the http request pointer. Checks if the class has been init. If 
// not, returns false and a log entry. If true, returns true. Prevents public 
// functions from working if not init.
bool OTAHAND::initCheck(httpd_req_t* req) {
    if (req == nullptr) return false;

    if (!OTAHAND::isInit) { // If not init reply to client and log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Not init", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);

        // Always send string, since this is a blocking function.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "initCk");
    }

    return OTAHAND::isInit;
}

// Reuires the ref/addr to the OTA handler object. Once init, ota will have
// functionality. Returns true if init, and false if not.
bool OTAHAND::init(OTA::OTAhandler &ota) {

    // Attempt to set.
    OTAHAND::OTA = &ota;

    // Check to ensure set and not nullptr
    OTAHAND::isInit = OTAHAND::OTA != nullptr;

    if (!isInit) { // If not init, send CRITICAL log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s not init", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);

    } else { // is Init, send INFO log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s init", 
            OTAHAND::tag);

        OTAHAND::sendErr(OTAHAND::log, Messaging::Levels::INFO);
    }

    return OTAHAND::isInit;
}

// Exclusive to OTA updates via LAN. The request is passed, and the 
// query string will contain only url, not sigurl, so only firmware
// url is used within the processing. Calls to update via OTA, and 
// returns ESP_OK regardless of integrity, since errors are handled
// via responses.
esp_err_t OTAHAND::updateLAN(httpd_req_t* req) {

    if (!OTAHAND::initCheck(req)) return ESP_OK; // Block code, OK required.

    // struct that holds both firmware and signature URLs. memsets upon init.
    OTA::URL LANurl; 
  
    esp_err_t set = httpd_resp_set_type(req, "text/html");

    if (set != ESP_OK) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "updLAN");

        return ESP_OK; // Block
    }

    // Check to ensure OTA is not nullptr, and extracts the URL into the LANurl
    // object. If the extract returns true, the url passes white list checks,
    // and this block runs.
    if (OTAHAND::OTA != nullptr && extractURL(req, LANurl, URLSIZE)) {

        // Sends the url IP only. This version only produces a firmware URL.
        // Copes the fw url to the sig followed by a concat of the appropriate
        // endpoints.
        strcpy(LANurl.signature, LANurl.firmware); // should be equal.
        strcat(LANurl.firmware, LAN_OTA_FIRMWARE_PATH);
        strcat(LANurl.signature, LAN_OTA_SIG_PATH);

        // Runs OTA update using the LANurl parameters. Upon success, responds
        // to client and restarts. 
        if (OTAHAND::OTA->update(LANurl, true) == OTA::OTA_RET::OTA_OK) {

            // Prevents the client from waiting for a response and making a 
            // second request. No log required.
            if (MASTERHAND::sendstrErr(httpd_resp_sendstr(req, "OK"), 
                OTAHAND::tag, "updLAN")) {

                vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to allow response
                esp_restart(); // Restart after sending.
            };

        } 
    } 

    // Either restarts or sends this message.
    snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),"%s OTA not updated", 
        OTAHAND::tag);

    MASTERHAND::sendErr(MASTERHAND::log);

    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
        OTAHAND::tag, "updLAN");

    return ESP_OK; // Must return ESP_OK
}

// Exclusive to OTA updates via web. The request is passed, and the 
// query string contains both url and sigurl. Calls to update via
// OTA, and returns ESP_OK regardless of integrity.
esp_err_t OTAHAND::update(httpd_req_t* req) {

    if (!OTAHAND::initCheck(req)) return ESP_OK; // Block code, OK required.

    // struct that holds both firmware and signature URLs. memsets upon init.
    OTA::URL WEBurl; 

    esp_err_t set = httpd_resp_set_type(req, "text/html"); // Set text

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "updWEB");

        return ESP_OK; // Block
    }

    // If the domain is in the white list, it will proceed with the OTA update.
    if (OTAHAND::OTA != nullptr && extractURL(req, WEBurl, URLSIZE)) {

        if (OTAHAND::OTA->update(WEBurl, false) == OTA::OTA_RET::OTA_OK) {
            
            if (MASTERHAND::sendstrErr(httpd_resp_sendstr(req, "OK"), 
                OTAHAND::tag, "updWEB")) {

                vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to allow response
                esp_restart(); // Restart after sending.
            };
        } 
    } 

    // Either restarts or sends this message.
    snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s OTA not updated", 
        OTAHAND::tag);

    MASTERHAND::sendErr(MASTERHAND::log);

    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
        OTAHAND::tag, "updWEB");

    return ESP_OK; // Must return ESP_OK
}

// This will be used to roll the OTA back to the previous version. Returns 
// ESP_OK after accepting request.
esp_err_t OTAHAND::rollback(httpd_req_t* req) {

    if (!OTAHAND::initCheck(req)) return ESP_OK; // Block code, OK required.

    esp_err_t set = httpd_resp_set_type(req, "text/html");

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "rollback");

        return ESP_OK; // Block
    }

    if (OTAHAND::OTA != nullptr) {
        
        // sends an OK string, rollsback, and then restarts the device.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, "OK"), OTAHAND::tag, 
            "rollbk");

        if (OTAHAND::OTA->rollback()) esp_restart();

    } else { // Is nullptr

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s OTA not updated", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "updLAN");

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, "FAIL"), OTAHAND::tag, 
            "rollBk");
    }

    return ESP_OK; // Must return ESP_OK
}

// Requires the http request. Checks if a new version of firmware is available.
// IF so, fills buffer with json data, and returns the parsed jason object.
// Processes data to check firmware version. If version is different than the
// current version, responds with the json string from the https web server.
// Returns ESP_OK or ESP_FAIL.
esp_err_t OTAHAND::checkNew(httpd_req_t* req) { // Handler for new FW version.

    if (!OTAHAND::initCheck(req)) return ESP_OK; // Block code, OK required.

    esp_err_t set = httpd_resp_set_type(req, "application/json"); // set type

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "chkNew");

        return ESP_OK; // Block
    }

    // Ensures that this is in station mode, to prevent checks while in WAP mode
    if (NetMain::getNetType() != NetMode::STA) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s FW chk must be STA mode", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        // wap version lets client know in WAP mode.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
            "{\"version\":\"wap\"}"), OTAHAND::tag, "chkNew");

        return ESP_OK; // required by uri reg.
    }

    // If all is good, continue below.

    char buffer[OTA_CHECKNEW_BUFFER_SIZE]{0};

    // ENSURE THAT WE ARE CHECKING VARIABLES BEFORE CONTINUING ON, SUCH AS JSON
    // TO ENSURE THAT IS IS NOT SET TO NULL, ALTHOUGH PROCESS JSON COVERS THIS 
    // IN ITS FIRST CHECK, WE SHOULD DO PRE-EMPTIVE CHECKS AND THEN HANDLE IT
    // HERE BEFORE MAKING ANOTHER THING RELY. ONCE A STRING IS SENT, IT SHOULD 
    // END THERE AND MAYBE RETURN SOME SORT OF VALUE. ENSURE THAT THIS IS CONSISTENT
    // THROUGH ALL HANDLERS. THIS IS EVIDENT WHEN THE SERVER IS NOT RUNNING, IT
    // PRINTS 4 LOG ENTRIES. SEE SERIAL MONITOR.

    // returns parsed JSON string.
    cJSON* json = receiveJSON(req, buffer, sizeof(buffer));
    cJSON* version{NULL}; // Set to null temporarily.
 
    // Processes the json to extract the firmware version.
    if (OTAHAND::processJSON(json, req, &version) != ESP_OK) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Unable to process JSON", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // ATTENTION: No response string, will be handled in respondJSON.
    }

    // Once processed, responds to server with buffer data if firmware is new. 
    // If not, it will send a "match" response which will not show an update 
    // on the webpage. This can be ran at the bottom because if any of this
    // passed data is bad, a response will be sent to the server regardless.
    OTAHAND::respondJSON(req, &version, buffer);
    cJSON_Delete(json);

    return ESP_OK; // Must return ESP_OK
}

}