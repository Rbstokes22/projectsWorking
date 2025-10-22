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
#include "Common/FlagReg.hpp"
#include "Peripherals/saveSettings.hpp"
#include "Common/heartbeat.hpp"

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

    } else { // Bad whitelist check.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s whitelist chk bad", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MASTERHAND::log), 
            OTAHAND::tag, "extrurl");

        return false;
    }
}

// Requires the http request, buffer to be written to, and the buffer size.
// Sends the request to the web server. This will copy the received JSON
// into the passed buffer. Returns the parsed buffer as a cJSON* pointer,
// or NULL if error. ATTENTION!!! This function is exclusive to checking for
// OTA updates and not the update itself, which is managed in this src file,
// but occurs in the OTA class. 
cJSON* OTAHAND::receiveJSON(httpd_req_t* req, char* buffer, size_t size) {

    // ATT: Since request pointer is passed, Ensure a failure responses with
    // a string and log entry. In init and open client, this will be handled.

    // Use WEBURL from config.hpp, and concat it with the endpoint URL.
    char url[OTA_URL_SIZE]{0};
    strcpy(url, WEBURL);
    strcat(url, OTA_VERSION_PATH);
    esp_err_t err = ESP_FAIL;
    esp_http_client_handle_t client = NULL; // Establish client.

    // Static declaration here is fine as opposed to a static class var.
    // Just requires passing as reference vs directly referencing the global.
    static Flag::FlagReg flag("(OTAHANDFlag)"); // Used to handle cleanup.

    // Client configuration with the crt bundle attached for webserver 
    // cert validation. This is provided by esp and uses mozillas data.
    // When DEVmode changes to false, it will be handled by the server.
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .timeout_ms = WEB_TIMEOUT_MS,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    if (!OTAHAND::initClient(client, config, flag, req)) { // Attempt init.
        return NULL; // Err handling within function. Allows caller to err chk.
    }

    if (DEVmode) { // used for development mode only, req by ngrok.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    if (!OTAHAND::openClient(client, flag, req)) { // Attempt open.
        return NULL; // Err handling within function.
    }

    // Get the payload length through the headers.
    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) { // Indicates error.
        OTAHAND::cleanup(client, flag);

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Invalid content length of %d. %s", OTAHAND::tag, contentLength,
            esp_err_to_name(err));

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_INVAL_CONT_LEN),
            OTAHAND::tag, "recJSON");

        return NULL; // return NULL if error.
    }

    // Fills the passed buffer with the JSON data read from web server.
    int dataRead = esp_http_client_read(client, buffer, size);
    buffer[dataRead] = '\0'; // null terminate the buffer.

    OTAHAND::cleanup(client, flag); // Run a final cleanup if good.

    if (dataRead < 0) { // Indiates error.

        snprintf(OTAHAND::log, sizeof(OTAHAND::log), "%s client read err", 
            OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_CLI_READ_ERR),
            OTAHAND::tag, "recJSON");

        return NULL;
    } 
 
    return cJSON_Parse(buffer); // Return parsed buffer if data is solid.
}

// Once data is read into the buffer, the parsed json pointer is passed
// along with the request and the version. Uses a double pointer, since 
// the version is a pointer to a pointer. Sets the version to point to 
// the version in the JSON string. Returns ESP_OK for valid JSON data,
// and ESP_FAIL for invalid.
bool OTAHAND::processJSON(cJSON* json, httpd_req_t* req, cJSON** version) {

    // Double pointers passed to pointer to a pointer. This allows mod
    // to the original cJSON pointer by allowing it to point to a new
    // cJSON object. If a single pointer was passed, in the get object item,
    // the assignment would only change the local copy of the pointer. This
    // is because the get object item returns a pointer.

    if (json == NULL || json == nullptr) { // SHOULD NEVER OCCUR!!!

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s JSON NULL", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_JSON_INVALD), 
            OTAHAND::tag, "procjson");

        return false; // Block remaining code.
    }

    // Sets the version to point at the cJSON object with key = version.
    *version = cJSON_GetObjectItem(json, "version"); // Set by server

    // Checks that the version exists and is a string. Returns OK if good,
    // if not good, logs and responds to client.
    if (*version != NULL && cJSON_IsString(*version)) {
        return true; // Data is solid.

    } else { // Version does not exist or is not a string.

        *version = NULL; // Ensure version set back to NULL.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Unable to get firmware version", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_JSON_INVALD), 
            OTAHAND::tag, "procjson");

        return false;
    }
}

// Following the JSON processing, the version and buffer are passed along with
// the request. Compares the version passed, to the cur firmware version that 
// is kept in the config.hpp header. If they are equal, returns to the client, a
// version match. If they are not equal, sends entire response buffer from the 
// web server to the client for parsing, indicating an updatable version. If the
// data is corrupt, sends a response of invalid JSON. Returns ESP_OK.
bool OTAHAND::respondJSON(httpd_req_t* req, cJSON** version, 
    const char* buffer) {

    // Checks if the version is different than the current version. If 
    // so it sends back the url data for the firmware and signatures.

    // This is checked in the processJSON. This should always be true when
    // getting to this point. This checks that the version offered is different
    // than the current version. If different, returns true and sends back the
    // url data for the firmware and signature. If not, returns false, and
    // logs. 
    if (*version != NULL && cJSON_IsString(*version)) {
        char _version[20]{0}; // Will never exceed this size.
        snprintf(_version, sizeof(_version), "%s", (*version)->valuestring);

        if (strcmp(_version, FIRMWARE_VERSION) == 0) { // Match, no update req
        
            // No log required. Only response string.
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_OTA_FW_MATCH), 
                OTAHAND::tag, "respjson");

        } else { // No match, respond with buffer indicating availability.

            // buffer is in JSON format.
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, buffer), 
                OTAHAND::tag, "procjson");

            return true;
        }

    } else { // Bad data passed.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Bad JSON response data", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_JSON_INVALD), 
            OTAHAND::tag, "respjson");
    }
    
    return false; // Indicates no changes necessary.
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

// Requires the references to the client, its configuration, and the flag
// object. Initializes client using config, and set INIT flag if successful.
// If not, cleans connection, and returns false. Returns true if successful.
// WARNING: Must call before calling openClient.
bool OTAHAND::initClient(esp_http_client_handle_t &client, 
    esp_http_client_config_t &config, Flag::FlagReg &flag, httpd_req_t* req) {

    // If not init, then init. Sets client = NULL if error.
    if (!flag.getFlag(OTAFLAGS::INIT)) {
        client = esp_http_client_init(&config);
    }
    
    // Check that client has been init by checking not NULL.
    if (client != NULL) { 
        flag.setFlag(OTAFLAGS::INIT); // Set flag to prevent re-init.
        return true;

    } else { // Client has not been init.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Connection unable to init", OTAHAND::tag);
        
        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_CON_INIT_FAIL), 
            OTAHAND::tag, "cliInit");

        OTAHAND::cleanup(client, flag); // Logging captured in function.
        return false;
    }
}

// Requires references to client and flag. Opens client connection. Sets the 
// OPEN flag if successful, along with returning true. Returns false and
// cleans connection if unsuccesful. WARNING: Must call after client init.
bool OTAHAND::openClient(esp_http_client_handle_t &client, 
    Flag::FlagReg &flag, httpd_req_t* req) {

    // WARNING. This function, when unresolved, will cause the heartbeat to
    // reset. This will extend all heartbeats by the timeout, in seconds to
    // prepare for a blocking function, only when needed, like this known case.
    // This is used exclusively here and will not effect any of the other
    // hearbeats.

    esp_err_t err = ESP_FAIL; // Default if conditions are not met below.

    // Opens the connection if not already open and has been init.
    if (!flag.getFlag(OTAFLAGS::OPEN) && flag.getFlag(OTAFLAGS::INIT)) {

        // As a prepatory measure, extend heartbeat times before opening client.
        heartbeat::Heartbeat* HB = heartbeat::Heartbeat::get();
        if (HB == nullptr) {
            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
                "%s nullptr passed", OTAHAND::tag);

            MASTERHAND::sendErr(MASTERHAND::log);
            return false;
        }

        // Extend heartbeats as a prepatory measure, by the seconds floored + 1.
        uint8_t HB_SEC = (WEB_TIMEOUT_MS / 1000) + 1;

        HB->extendAll(HB_SEC); // Extend before potential blocker.

        err = esp_http_client_open(client, 0);

        HB->clearExtAll(); // Clear extensions after potential blocker.
    }
 
    if (err == ESP_OK) { // Has been open, set flag.
        flag.setFlag(OTAFLAGS::OPEN);
        return true;

    } else { // Invoked if above fails due to setting err = ESP_FAIL.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Connection unable to open. %s", OTAHAND::tag, 
            esp_err_to_name(err));

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::WARNING);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_CON_OPEN_FAIL), 
            OTAHAND::tag, "cliOpen");

        OTAHAND::cleanup(client, flag);
        return false;
    }
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
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_NOT_INIT), 
            OTAHAND::tag, "initCk");
    }

    return OTAHAND::isInit;
}

// Requires references to the clients and flags, and the attempt which is 
// default to 0. This is for internal handling and should only be call with a
// number other than 0 internally. Closes and cleans the client connections.
// Returns true if clean, and false if not. Cleans iff the flag has been set.
bool OTAHAND::cleanup(esp_http_client_handle_t &client, Flag::FlagReg &flag, 
    size_t attempt) {

    attempt++; // Incremenet the passed attempt. This is to handle retries.

    if (attempt >= OTA_CLEANUP_ATTEMPTS) {
      
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Unable to close connection, restarting", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);
        NVS::settingSaver::get()->save(); // Save peripheral settings

        esp_restart(); // Restart esp attempt.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), // Log if failure.
            "%s Unable to restart", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);
        return false; // Just in case it doesnt restart.
    }

    // Default to ESP_OK in the event that one or both flags are not set.
    esp_err_t close{ESP_OK}, cleanup{ESP_OK}; 

    if (flag.getFlag(OTAFLAGS::OPEN)) { // Client has been opened.
        close = esp_http_client_close(client); // Attempt client close.

        if (close != ESP_OK) { 
            
            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                "%s Unable to close client. %s", OTAHAND::tag,
                esp_err_to_name(close));

            MASTERHAND::sendErr(MASTERHAND::log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            OTAHAND::cleanup(client, flag, attempt); // Retry
    
        }

        flag.releaseFlag(OTAFLAGS::OPEN); // Release if closed.
    }

    if (flag.getFlag(OTAFLAGS::INIT)) { // Client has been init.
        cleanup = esp_http_client_cleanup(client); // Attempts to cleanup.

        if (cleanup != ESP_OK) {

            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                "%s Unable to cleanup client. %s", OTAHAND::tag, 
                esp_err_to_name(cleanup));

            MASTERHAND::sendErr(MASTERHAND::log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            OTAHAND::cleanup(client, flag, attempt);
        }

        flag.releaseFlag(OTAFLAGS::INIT); // Release if cleaned up.
    }

    return (close == ESP_OK && cleanup == ESP_OK); // Typically a success.
}

// Requires request ptr. Runs a station check. Returns true if connected to 
// station, and false if not while alsosending http response.
bool OTAHAND::STAcheck(httpd_req_t* req) {
    if (NetMain::getNetType() != NetMode::STA) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s FW chk must be STA mode", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Always send string, since this is a blocking function.
        // wap version lets client know in WAP mode.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_OTA_JSON_WAP), 
            OTAHAND::tag, "STAchk");

        return false; 
    }

    return true; // Indicates connected to STA mode.
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

    // struct that holds both firmware and signature URLs. memsets upon init.
    OTA::URL LANurl; 
  
    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON);

    if (set != ESP_OK) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Attempt to send string, despite type not being set. It might
        // register as something to the client, and prevent them from waiting.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL), 
            OTAHAND::tag, "updLAN");

        return ESP_OK; // Block
    }

    // Block code, OK required. Err handling within function.
    if (!OTAHAND::initCheck(req)) return ESP_OK; 
    if (!OTAHAND::STAcheck(req)) return ESP_OK; // Ensure STA conn.

    // Check to make sure not nullptr. Should never happen.
    if (OTAHAND::OTA != nullptr) { // Redundancy. Split to handle err resp.

        // Separated from if check above. This func handles errors within.
        if (extractURL(req, LANurl, URLSIZE)) {

            // Sends the url IP only. This version only produces a firmware URL.
            // Copies the fw url to the sig followed by a concat of the approp
            // endpoints.
            strcpy(LANurl.signature, LANurl.firmware); // should be same.
            strcat(LANurl.firmware, LAN_OTA_FIRMWARE_PATH);
            strcat(LANurl.signature, LAN_OTA_SIG_PATH);

            // Runs OTA update using the LANurl parameters. Upon success, resp
            // to client and restarts. 
            if (OTAHAND::OTA->update(LANurl, true) == OTA::OTA_RET::OTA_OK) {

                // Prevents the client from waiting for a response and making a 
                // second request. No log required.
                if (MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
                    MHAND_SUCCESS), OTAHAND::tag, "updLAN")) {

                    vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to resp
                    NVS::settingSaver::get()->save(); // Save periph settings
                    esp_restart(); // Restart after sending.
                };

            } else { // OTA did not update.

                snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
                    "%s OTA upd fail", OTAHAND::tag);
    
                MASTERHAND::sendErr(MASTERHAND::log);
                MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL),
                    OTAHAND::tag, "updLAN");

                return ESP_OK; // Block
            }
        } // Else not required. Error handling in extractURL func.
    }

    // Either restarts, returns, or sends this message.
    snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),"%s OTA not updated", 
        OTAHAND::tag);

    MASTERHAND::sendErr(MASTERHAND::log);

    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL), 
        OTAHAND::tag, "updLAN");

    return ESP_OK; // Must return ESP_OK
}

// Exclusive to OTA updates via web. The request is passed, and the 
// query string contains both url and sigurl. Calls to update via
// OTA, and returns ESP_OK regardless of integrity.
esp_err_t OTAHAND::update(httpd_req_t* req) {

    // struct that holds both firmware and signature URLs. memsets upon init.
    OTA::URL WEBurl; 

    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON); 

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Attempt to send string, despite type not being set. It might
        // register as something to the client, and prevent them from waiting.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL), 
            OTAHAND::tag, "updWEB");

        return ESP_OK; // Block
    }

    // Block code, OK required. Err handling within function.
    if (!OTAHAND::initCheck(req)) return ESP_OK; 
    if (!OTAHAND::STAcheck(req)) return ESP_OK; // Ensure STA conn.

    if (OTAHAND::OTA != nullptr) { // Redundancy. Split to handle err response.

        // extractURL handles error responding and logging within.
        if (extractURL(req, WEBurl, URLSIZE)) { // Good and in whitelist.

            if (OTAHAND::OTA->update(WEBurl, false) == OTA::OTA_RET::OTA_OK) {

                if (MASTERHAND::sendstrErr(httpd_resp_sendstr(req, 
                    MHAND_SUCCESS), OTAHAND::tag, "updWEB")) {

                    vTaskDelay(pdMS_TO_TICKS(500)); //Delay 500 ms to resp
                    NVS::settingSaver::get()->save(); // Save periph settings
                    esp_restart(); // Restart after sending.
                } // No else block required.

            } else { // OTA did not update.

                snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
                    "%s OTA upd fail", OTAHAND::tag);

                MASTERHAND::sendErr(MASTERHAND::log);
                MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL),
                    OTAHAND::tag, "updWEB");

                return ESP_OK; // Block
            }
        } // Else not required, err handling within the extractURL func.
    }

    // Either restarts, returns, or sends this message.
    snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s OTA not updated", 
        OTAHAND::tag);

    MASTERHAND::sendErr(MASTERHAND::log);

    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL), 
        OTAHAND::tag, "updWEB");

    return ESP_OK; // Must return ESP_OK
}

// This will be used to roll the OTA back to the previous version. Returns 
// ESP_OK after accepting request.
esp_err_t OTAHAND::rollback(httpd_req_t* req) {

    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON);

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Attempt to send string, despite type not being set. It might
        // register as something to the client, and prevent them from waiting.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL), 
            OTAHAND::tag, "rollbk");

        return ESP_OK; // Block
    }

    // Block code, OK required. Err handling within function.
    if (!OTAHAND::initCheck(req)) return ESP_OK;

    if (OTAHAND::OTA != nullptr) { // Checks, but should never fail.

        if (OTAHAND::OTA->rollback()) {
            // sends an OK string, rollsback, and then restarts the device.
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_SUCCESS), 
                OTAHAND::tag, "rollbk");

            esp_restart();

        } else { // Unable to rollback.

            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                "%s Unable to roll back", OTAHAND::tag);

            MASTERHAND::sendErr(MASTERHAND::log);
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL),
                OTAHAND::tag, "rollbk");
        }
        
    } else { // Is nullptr

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s OTA not updated. Nullptr.", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL), 
            OTAHAND::tag, "rollBk");
    }

    return ESP_OK; // Must return ESP_OK
}

// Requires the http request. Checks if a new version of firmware is available.
// IF so, fills buffer with json data, and returns the parsed jason object.
// Processes data to check firmware version. If version is different than the
// current version, responds with the json string from the https web server.
// Returns ESP_OK or ESP_FAIL.
esp_err_t OTAHAND::checkNew(httpd_req_t* req) { // Handler for new FW version.

    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON); 

    if (set != ESP_OK) {
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s http resp type not set", OTAHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Attempt to send string, despite type not being set. It might
        // register as something to the client, and prevent them from waiting.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL), 
            OTAHAND::tag, "chkNew");

        return ESP_OK; // Block
    }

    // Block code, OK required. Err handling within function.
    if (!OTAHAND::initCheck(req)) return ESP_OK; 
    if (!OTAHAND::STAcheck(req)) return ESP_OK; // Ensure STA conn.

    // If all is good, continue below.

    char buffer[OTA_CHECKNEW_BUFFER_SIZE]{0};

    // ATTENTION: Chain with receive, process, and respond JSON.

    // returns parsed JSON string. All responses and error handling is within
    // the function.
    cJSON* json = receiveJSON(req, buffer, sizeof(buffer));

    // Check string for validity
    if (json == NULL || json == nullptr) {
        cJSON_Delete(json); // Del before return.
        return ESP_OK; // required by URI reg. BLOCKS remaining code.
    }

    // Set to null temporarily. Will populate with firmware version.
    cJSON* version{NULL}; 

    // if JSON valid, processes the new JSON. Checks to ensure that version is
    // located within the JSON, if not/NULL, handles errors and logging within.
    if (!OTAHAND::processJSON(json, req, &version)) {
        cJSON_Delete(json); // Del before return.
        return ESP_OK; // Required by URI reg. Err handling within function.
    }

    // If JSON processed, responds to the client with the buffer data if new
    // firmware avail. If not, sends client information indicating invalid JSON,
    // or a match. All error handling within function, no bool check req.
    OTAHAND::respondJSON(req, &version, buffer);
    cJSON_Delete(json);

    return ESP_OK; // Must return ESP_OK
}

}