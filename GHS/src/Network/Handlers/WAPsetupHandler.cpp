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
NVS::Creds* WSHAND::creds{nullptr};
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
        if (isInit) WSHAND::wap->sendErr("JSON request Error", errDisp::SRL);
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

        if (httpd_resp_sendstr(req, "{\"status\": \"Invalid JSON\"}") != ESP_OK) {
            if (isInit) {
                WSHAND::wap->sendErr("Invalid JSON response", errDisp::SRL);
            }
        }

        return ESP_FAIL;
    }

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
                
            if (strcmp(key, "ssid") == 0) {
                WSHAND::station->setSSID(item->valuestring);
            } else if (strcmp(key, "pass") == 0) {
                WSHAND::station->setPass(item->valuestring);
            } else if (strcmp(key, "phone") == 0) {
                WSHAND::station->setPhone(item->valuestring);
            } else if (strcmp(key, "WAPpass") == 0) {
                WSHAND::wap->setPass(item->valuestring);
            }

            // Will write to NVS as this method's primary objective.
            NVS::nvs_ret_t stat = WSHAND::creds->write(
                key, item->valuestring, strlen(item->valuestring)
                );

            if (stat == NVS::nvs_ret_t::NVS_WRITE_OK) {
                strcpy(writtenKey, key); // No issue with sizing.
            } // remains NULL if write didnt work.

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
    cJSON* response = cJSON_CreateObject();
    char respStr[35] = "Not Accepted"; // Default.
    bool markForDest{false}; // destruction flag

    // If the WAPpass is changed, this will trigger a destruction and 
    // re-initialization of the connection with the new password. The
    // exact respStr is what the webpage is expecting. Do not change 
    // without changing the webpage expected value/string.
    if (strcmp(writtenKey, "WAPpass") == 0) {
        strcpy(respStr, "Accepted, Reconnect to WiFi");
        markForDest = true;
    } else if (strcmp(writtenKey, "NULL") == 0) {
        strcpy(respStr, "Not Accepted"); // redundancy
    } else {
        strcpy(respStr, "Accepted");
    }

    bool addObj = cJSON_AddItemToObject(
            response, 
            "status", // key that the webpage is expecting.
            cJSON_CreateString(respStr)
            );

    if (!addObj && isInit) {
        WSHAND::wap->sendErr("JSON response not added", errDisp::SRL);
    }

    // converts json response to a json string.
    char* responseJSON = cJSON_Print(response);

    // Sends JSON response to webpage.
    if (httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN) != ESP_OK 
        && isInit) {

        WSHAND::wap->sendErr("Error sending response", errDisp::SRL);
    }
    
    // deletes json object and frees char pointer.
    if (response != NULL) cJSON_Delete(response);
    if (responseJSON != NULL) free(responseJSON);
    
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
    NetWAP &wap, 
    NVS::Creds &creds) {

    WSHAND::station = &station;
    WSHAND::wap = &wap;
    WSHAND::creds = &creds;
    
    if (WSHAND::station != nullptr &&
        WSHAND::wap != nullptr &&
        WSHAND::creds != nullptr) {

        WSHAND::isInit = true;
    }

    return WSHAND::isInit;
}

// Main index handler.
esp_err_t WSHAND::IndexHandler(httpd_req_t *req) {
    if (httpd_resp_set_type(req, "text/html") != ESP_OK && isInit) {
        WSHAND::wap->sendErr("Error setting response type", errDisp::SRL);
    }

    if (httpd_resp_send(req, WAPSetupPage, strlen(WAPSetupPage)) != ESP_OK 
        && isInit) {

        WSHAND::wap->sendErr("Error sending response", errDisp::SRL);
    }

    return ESP_OK; // Must return or socket will shut down.
}

// Main handler that will handle and process the request, responding
// to the client with a json.
esp_err_t WSHAND::DataHandler(httpd_req_t* req) { 
    if (httpd_resp_set_type(req, "application/json") != ESP_OK && isInit) {
        WSHAND::wap->sendErr("Error setting response type", errDisp::SRL);
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