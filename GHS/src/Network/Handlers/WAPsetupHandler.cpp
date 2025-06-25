#include "Network/Handlers/WAPsetupHandler.hpp"
#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/MasterHandler.hpp"

namespace Comms {

// Static setup. Passed through init function.
const char* WSHAND::tag("(WSHAND)");
NetSTA* WSHAND::station{nullptr};
NetWAP* WSHAND::wap{nullptr};
bool WSHAND::isInit{false};

// Requires http request. Receives JSON request, and creates a buffer that 
// stores the data. Returns the JSON parsed buffer. The JSON data will include
// a key value pair.
cJSON* WSHAND::receiveJSON(httpd_req_t* req) {
    char buffer[WAPHANDLER_JSON_SIZE]{0}; 
    int recvSize{0}, totalLength{0};

    // Steps:
    // 1) Sets recvSize to equal the bytes of the request return.
    // 2) While the size is > 0, append received bytes to the 
    //    buffer.
    // 3) Updates the totalLength to reflect the total bytes
    //    recived.
    // 4) Ensure the buffer is null terminated for proper
    //    parsing.
    while ((recvSize = httpd_req_recv(req, // Request
        buffer + totalLength, // next buffer address
        sizeof(buffer) - totalLength - 1) // buffer remaining length
        ) > 0) { // is greater than 0.

        totalLength += recvSize; // Add the size to the total current length.
    }

    buffer[totalLength] = '\0'; // Ensure null term once complete.

    if (recvSize < 0 || totalLength <= 0) { // indicates error 

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s JSON recv Error", 
            WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_JSON_RCV_ERR), 
            WSHAND::tag, "recvJson");

        return NULL;
    }
    
    return cJSON_Parse(buffer); // Return parsed buffer if successful.
}

// Requires the cJSON pointer, the http request, and the writtenKey pointer.
// The writtenKey will be overwritten with the key that was written to NVS.
// Returns ESP_OK or ESP_FAIL.
bool WSHAND::processJSON(cJSON* json, httpd_req_t* req, char* writtenKey) {

    // Redundant, should never occur.
    if (json == NULL || json == nullptr) { 

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s JSON NULL", 
            WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_JSON_INVALD), 
            WSHAND::tag, "procjson");

        return false; // Block remaining code.
    }

    // If params are solid, continue.

    // SMS requirements for alerts. Pass true to bypass validation to return
    // the struct in order to manipulate. Only used for phone and API key.
    NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq(true);

    // Iterates the netKeys from netConfig.hpp. These are the only
    // acceptable keys within this scope. Any changes to the config, must 
    // reflect here.
    for (auto &key : netKeys) {

        // Looks in the json buffer for the selected key in the iteration 
        // process. If it is not, it will have a valuestring of NULL and
        // will not be a string. This will continue on until a match exists.
        cJSON* item = cJSON_GetObjectItem(json, key); 
        
        // Acts as a redundancy in the event the NVS fails. It will set
        // the correct data in the running system, to allow access to 
        // the station as well as allow WAP password changes.

        // Upon a match, process to set the class variables and write to NVS.
        if (cJSON_IsString(item) && (item->valuestring != NULL)) { // EXISTS

            size_t valueLen = strlen(item->valuestring); // length
            char* value = item->valuestring; // char value of string
            bool meetsReq{false}; // meets writing requirement to NVS
                
            // ATTENTION: The SMS requirements were built at a later time,
            // which is why the style is different than the other setters. No
            // req to rebuild.
            if (strcmp(key, "ssid") == 0) { 
                // less than or equal to 32 is the requirement.
                if (valueLen < static_cast<int>(IDXSIZE::SSID)) {
                    WSHAND::station->setSSID(value);
                    meetsReq = true;
                }

            } else if (strcmp(key, "pass") == 0) {
                // less than or equal to 64 is the requirement
                if (valueLen < static_cast<int>(IDXSIZE::PASS)) {
                    WSHAND::station->setPass(value);
                    meetsReq = true;
                }
                
            } else if (strcmp(key, "WAPpass") == 0) {
                // less than or equal to 64 is the requirement
                if (valueLen < static_cast<int>(IDXSIZE::PASS)) {
                    WSHAND::wap->setPass(value);
                    meetsReq = true;
                }
    
            } else if ((strcmp(key, "phone") == 0) && (sms != nullptr)) {
                // equal to exactly 10 with no letters is the requirement.
                if (valueLen == (static_cast<int>(IDXSIZE::PHONE) - 1)) {
                    snprintf(sms->phone, sizeof(sms->phone), "%s", value);
                    meetsReq = true;
                }

            // A bit longer check since we have to check for proper HEX format.
            } else if ((strcmp(key, "APIkey") == 0) && (sms != nullptr)) {
                // equal to exactly 8 with characters within the scope
                // of hexadecimal format. Allows 4B+ keys.
                if (valueLen == (static_cast<int>(IDXSIZE::APIKEY) - 1)) {
                    bool isHex = false; // Used to determine if hex format.

                    // Iterate each character and ensure it is hex format.
                    for (int i = 0; i < valueLen; i++) {
                        uint8_t val = value[i];

                        // Check is char is a valid hexadecimal
                        bool numeric = (val >= '0' && val <= '9');
                        bool lwrAlpha = (val >= 'a' && val <= 'f');
                        bool uprAlpha = (val >= 'A' && val <= 'F');

                        // If no hits, sets isHex to false.
                        bool isHex = (numeric || lwrAlpha || uprAlpha);
                        if (!isHex) break; // Break if non-hex val.
                    }

                    if (isHex) { // Copy APIkey over if correct format.
                        snprintf(sms->APIkey, sizeof(sms->APIkey), "%s", value);
                        meetsReq = true; // Set to true since written.
                    }
                }
            }

            // If meets requirement, will write to the NVS as this method's
            // primary object iff the correct format is entered. This prevents
            // malicious attempts to modify JS controls. Returns true or false
            // depending on success.
            return WSHAND::handleNVS(meetsReq, key, item, req, writtenKey);
        } 

        // not req, just written to show the iteration continues until key
        // is matched.
        continue; 
    }

    // If all keys are iterated with no match, send err and return false.
    snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s json key not found", 
        WSHAND::tag);

    MASTERHAND::sendErr(MASTERHAND::log);
    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_KEY_NOT_FOUND),
        WSHAND::tag, "procJson");

    return false; // Key not found.
}

// Once the settings have been updated and the NVS is written to, 
// sends response to browser. 

// Requires http request, and the written key. Responds to the client once the
// settings have been updated and the NVS is written to. Will not run if NVS
// has not been written to.
bool WSHAND::respondJSON(httpd_req_t* req, char* writtenKey) {

    // Destruction flag. To use this function, must be in WAP Setup mode.
    // If WAP password is changed, will be set to true, causing restart.
    bool markForDest{false};
    char respStr[60]{0}; // Response string, size will suffice.

    // If the WAPpass is changed, this will trigger a destruction and 
    // re-initialization of the connection with the new password. The
    // exact respStr is what the webpage is expecting. Do not change 
    // without changing the webpage expected value/string.
    if (strcmp(writtenKey, "WAPpass") == 0) {
        strcpy(respStr, MHAND_JSON_RECON);
        markForDest = true;
    } else if (strcmp(writtenKey, WAPHANDLER_WRITTEN_KEY) == 0) {
        strcpy(respStr, MHAND_JSON_NOT_ACC); // redundancy
    } else {
        strcpy(respStr, MHAND_JSON_ACC);
    }

    // Respond to webpage
    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, respStr), WSHAND::tag, 
        "respJson");

    // If true, sets the WAP type to WAP_RECON in order to force the 
    // reconnection in the NetManager with new password.
    if (markForDest) { 
        WSHAND::wap->setNetType(NetMode::WAP_RECON);
    }

    return true;
}

// Requires http request. Ensures that the class has been init. Returns true
// if yes, and false if not.
bool WSHAND::initCheck(httpd_req_t* req) {
    if (req == nullptr) return false;

    if (!WSHAND::isInit) { // If not init reply to client and log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Not init", WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_NOT_INIT), 
            WSHAND::tag, "initCk");
    }

    return WSHAND::isInit;
}

// Requires boolean to write or not write, the JSON key, the JSON item, the
// http request, and the written key to be modified by the actual key. Attempts
// a write to NVS if conditions are met. Returns true upon a successful write,
// and false upon failure.
bool WSHAND::handleNVS(bool write, const char* key, cJSON* item,
    httpd_req_t* req, char* writtenKey) {

    if (!write) { // never met requirements to update variable nor write.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s cred not written to NVS. Class var is not set", 
            WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL),
            WSHAND::tag, "procJson");

        return false;
    } 
    
    // If requirements are met, attempt a store into the NVS.
            
    NVS::nvs_ret_t stat = NVS::Creds::get()->write(key, 
        item->valuestring, strlen(item->valuestring) + 1);

    if (stat == NVS::nvs_ret_t::NVS_WRITE_OK) { // Written to NVS

        // Write the key written into writtenKey variable for later comparison.
        snprintf(writtenKey, WAPHANDLER_WRITTEN_KEY_SIZE, "%s", key);

        // Now log entry that cred has been written.
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s key %s updated in NVS", WSHAND::tag, key);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

        return true;

    } else { // Not written to NVS, class var still set.

        // Log that variable is set, but NVS did not write. This will still 
        // work until device is reset.
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s cred not written to NVS. Class var key %s is set", 
            WSHAND::tag, key);

        MASTERHAND::sendErr(MASTERHAND::log, 
            Messaging::Levels::WARNING);

        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_FAIL),
            WSHAND::tag, "procJson");

        // Will not affect setting, shows NVS not written to. Sends false to
        // stop further responses from being set.
        return false; 
    }
}

// Requires the station, wap, and creds objects. Initializes the address of 
// each to its pointer. Returns true or false if init.
bool WSHAND::init(NetSTA &station, NetWAP &wap) {

    // Attempt to set objects
    WSHAND::station = &station;
    WSHAND::wap = &wap;

    // Confirm setting by ensuring they are not nullptr.
    WSHAND::isInit = (WSHAND::station != nullptr) && (WSHAND::wap != nullptr);

    if (!isInit) { // if uninit, send CRITICAL log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s not init", 
            WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);

    } else { // If init, send INFO log.

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s init",
            WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);
    }

    return WSHAND::isInit;
}

// Requires http request. This is the main index handler and serves back
// the index page.
esp_err_t WSHAND::IndexHandler(httpd_req_t *req) {

    if (!WSHAND::initCheck(req)) return ESP_OK; // Blocks code if not init.

    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_TEXTHTML);

    if (set != ESP_OK) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Error setting response type", WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL_TEXT),
            WSHAND::tag, "idxHand");

        return ESP_OK; // Block.
    }

    // If resp type set, send page.
    MASTERHAND::sendstrErr(httpd_resp_sendstr(req, WAPSetupPage), WSHAND::tag,
        "idxHand");

    return ESP_OK; // Must return or socket will shut down.
}

// Requires http request. The is the handle that will process the data entered
// by the client on the WAP Setup page. This will respond to the client in json.
esp_err_t WSHAND::DataHandler(httpd_req_t* req) { 

    if (!WSHAND::initCheck(req)) return ESP_OK; // Blocks code if not init.

    esp_err_t set = httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON);

    if (set != ESP_OK) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s http response type not set", WSHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL),
            WSHAND::tag, "initCk");

        return ESP_OK; // block.
    }

    // If type is set, set written key. This will be used later to
    // determine if key has been written or not.
    char writtenKey[WAPHANDLER_WRITTEN_KEY_SIZE] = WAPHANDLER_WRITTEN_KEY; 

    cJSON* json = WSHAND::receiveJSON(req);

    if (json == NULL || json == nullptr) {
        cJSON_Delete(json);
        return ESP_OK; // Blocks, err handling within receiveJSON func.
    }

    // If JSON has been received, process.
    if (!WSHAND::processJSON(json, req, writtenKey)) {
        cJSON_Delete(json);
        return ESP_OK; // Blocks. Err handling within processJSON func.
    }

    // If JSON has been processed, respond to client.
    WSHAND::respondJSON(req, writtenKey);

    cJSON_Delete(json);

    return ESP_OK; // Must return or socket will shut down.
}

}