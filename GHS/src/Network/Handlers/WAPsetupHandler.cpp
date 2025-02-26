#include "Network/Handlers/WAPsetupHandler.hpp"
#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "cJSON.h"
#include "Network/webPages.hpp"
#include "Config/config.hpp"

namespace Comms {

// Static setup
NetSTA* WSHAND::station{nullptr};
NetWAP* WSHAND::wap{nullptr};
bool WSHAND::isInit{false};

// Receives the JSON request, and creates a buffer that 
// stores the data. Returns the JSON parsed buffer. 
cJSON* WSHAND::receiveJSON(httpd_req_t* req) {
    char buffer[100]{0}; // will never exceed.
    int ret{0}, totalLength{0};

    // Steps:
    // 1) Sets ret to equal the bytes of the request return.
    // 2) While the size is > 0, append received bytes to the 
    //    buffer.
    // 3) Updates the totalLength to reflect the total bytes
    //    recived.
    // 4) Ensure the buffer is null terminated for proper
    //    parsing.
    while ((ret = httpd_req_recv(
        req, 
        buffer + totalLength, // next buffer address
        sizeof(buffer) - totalLength - 1)) > 0) {

        totalLength += ret;
    }

    if (ret < 0) { // indicates error
        if (isInit) WSHAND::wap->sendErr("JSON request Error");
        return NULL;
    }
    
    buffer[totalLength] = '\0';

    return cJSON_Parse(buffer);
}

// Processes the parsed JSON buffer once received. The JSON buffer,
// request, and writtenKey is passed as arguments. The writtenKey will
// contain the key that is written to NVS. Returns ESP_OK or ESP_FAIL.
esp_err_t WSHAND::processJSON(cJSON* json, httpd_req_t* req, char* writtenKey) {
    if (json == NULL) {

        if (httpd_resp_sendstr(req, "{\"status\":\"Invalid JSON\"}") != ESP_OK) {
            if (isInit) {
                WSHAND::wap->sendErr("Invalid JSON response");
            }
        }

        return ESP_FAIL;
    }

    // SMS requirements for alerts. Pass true to bypass validation to manip.
    NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq(true);

    // Iterates the netKeys from netConfig.hpp. These are the only
    // acceptable keys in this scope.
    for (auto &key : netKeys) {
        cJSON* item = cJSON_GetObjectItem(json, key);
        
        // Acts as a redundancy in the event the NVS fails. It will set
        // the correct data in the running system, to allow access to 
        // the station as well as allow WAP password changes.
        if (
            cJSON_IsString(item) && isInit &&
            item->valuestring != NULL) {

            size_t valueLen = strlen(item->valuestring); // length
            char* value = item->valuestring; // char value of string
            bool meetsReq{false}; // meets writing requirement to NVS
                
            // The SMS requirements were built at a later time, which is why the
            // style is not the same as the other setters. There is no need to 
            // rebuild everything if already functioning.
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
                    strncpy(sms->phone, value, sizeof(sms->phone) - 1);
                    sms->phone[sizeof(sms->phone) - 1] = '\0';
                    meetsReq = true;
                }

            } else if ((strcmp(key, "APIkey") == 0) && (sms != nullptr)) {
                // equal to exactly 8 with characters within the scope
                // of hexadecimal format.
                if (valueLen == (static_cast<int>(IDXSIZE::APIKEY) - 1)) {
                    bool isHexFormat{true};

                    for (int i = 0; i < valueLen; i++) {
                        uint8_t val = value[i];

                        // Check is char is a valid hexadecimal
                        if (!((val >= '0' && val <= '9') ||
                            (val >= 'a' && val <= 'f') ||
                            (val >= 'A' && val <= 'F'))) {

                                isHexFormat = false;
                                break;
                            }
                    }

                    if (isHexFormat) {
                        strncpy(sms->APIkey, value, sizeof(sms->APIkey) - 1);
                        sms->APIkey[sizeof(sms->APIkey) - 1] = '\0';
                        meetsReq = true;
                    }
                }
            }
            
            // Will write to NVS as this method's primary objective if and only if
            // the correct format is entered in order to prevent malicious attempts
            // to modify javascript controls.
            if (meetsReq) {
                NVS::nvs_ret_t stat = NVS::Creds::get()->write(
                    key, 
                    item->valuestring, 
                    strlen(item->valuestring) + 1 // Null terminator
                    );

                if (stat == NVS::nvs_ret_t::NVS_WRITE_OK) {
                    strcpy(writtenKey, key); // No issue with sizing.
                } // remains NULL if write didnt work.
            }
            
            break;
        } 
    }

    // returns ESP_OK even if write didnt work. Error handling occurs in
    // the respondJSON and specifically looks for the written key. If 
    // NULL, it will mark it as unsuccessful and display that to the 
    // browser.  
    return ESP_OK;
}

// Once the settings have been updated and the NVS is written to, 
// sends response to browser. 
esp_err_t WSHAND::respondJSON(httpd_req_t* req, char* writtenKey) {
    char respStr[60]{0}; // Default.
    bool markForDest{false}; // destruction flag

    // If the WAPpass is changed, this will trigger a destruction and 
    // re-initialization of the connection with the new password. The
    // exact respStr is what the webpage is expecting. Do not change 
    // without changing the webpage expected value/string.
    if (strcmp(writtenKey, "WAPpass") == 0) {
        strcpy(respStr, "{\"status\":\"Accepted: Reconnect to WiFi\"}");
        markForDest = true;
    } else if (strcmp(writtenKey, "NULL") == 0) {
        strcpy(respStr, "{\"status\":\"Not Accepted\"}"); // redundancy
    } else {
        strcpy(respStr, "{\"status\":\"Accepted\"}");
    }

    // Sends JSON response to webpage.
    if (httpd_resp_sendstr(req, respStr) != ESP_OK && isInit) {
        WSHAND::wap->sendErr("Error sending response");
    }
    
    // If true, sets the WAP type to WAP_RECON in order to force the 
    // reconnection in the NetManager with new password.
    if (markForDest) { 
        WSHAND::wap->setNetType(NetMode::WAP_RECON);
    }

    return ESP_OK;
}

// Requires the station, wap, and creds objects. Initializes the
// address of each to its pointer. Returns true or false if init.
bool WSHAND::init(
    NetSTA &station, 
    NetWAP &wap) {

    WSHAND::station = &station;
    WSHAND::wap = &wap;
    
    if (WSHAND::station != nullptr &&
        WSHAND::wap != nullptr) {

        WSHAND::isInit = true;
    }

    return WSHAND::isInit;
}

// Main index handler.
esp_err_t WSHAND::IndexHandler(httpd_req_t *req) {
    if (httpd_resp_set_type(req, "text/html") != ESP_OK && isInit) {
        WSHAND::wap->sendErr("Error setting response type");
    }

    if (httpd_resp_send(req, WAPSetupPage, strlen(WAPSetupPage)) != ESP_OK 
        && isInit) {

        WSHAND::wap->sendErr("Error sending response");
    }

    return ESP_OK; // Must return or socket will shut down.
}

// Main handler that will handle and process the request, responding
// to the client with a json.
esp_err_t WSHAND::DataHandler(httpd_req_t* req) { 
    if (httpd_resp_set_type(req, "application/json") != ESP_OK && WSHAND::isInit) {
        WSHAND::wap->sendErr("Error setting response type");
    }

    char writtenKey[10] = "NULL";

    cJSON* json = WSHAND::receiveJSON(req);

    if (WSHAND::processJSON(json, req, writtenKey) == ESP_OK) {
        WSHAND::respondJSON(req, writtenKey);
    }

    if (json != NULL) cJSON_Delete(json);

    return ESP_OK; // Must return or socket will shut down.
}

}