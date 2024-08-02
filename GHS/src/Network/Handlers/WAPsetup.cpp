#include "Network/Handlers/WAPsetup.hpp"
#include "Network/webPages.hpp"
#include "esp_http_server.h"
#include "cJSON.h"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include <cstddef>

namespace Comms {

NetSTA *station;
NetWAP *wap;
NVS::Creds *creds;

void setJSONObjects(NetSTA &_station, NetWAP &_wap, NVS::Creds &_creds) {
    station = &_station;
    wap = &_wap;
    creds = &_creds;
}

esp_err_t WAPSetupIndexHandler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WAPSetupPage, strlen(WAPSetupPage));
    return ESP_OK;
}

esp_err_t WAPSubmitDataHandler(httpd_req_t *req) { 
    httpd_resp_set_type(req, "application/json");
    size_t arrSize{0};
    const char* keys[]{"ssid", "pass", "phone", "WAPpass"};
    char writtenKey[10] = "NULL";

    char buffer[100]{0};
    int ret, totalLength = 0;

    while ((ret = httpd_req_recv(
        req, 
        buffer + totalLength, 
        sizeof(buffer) - totalLength - 1)) > 0) {

        totalLength += ret;
    }
    
    buffer[totalLength] = '\0';

    cJSON* json = cJSON_Parse(buffer);

    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    for (auto &key : keys) {
        cJSON* item = cJSON_GetObjectItem(json, key);
     
        // Acts as a redundancy in the event the NVS fails. It will set
        // the correct data in the running system, to allow access to 
        // the station as well as allow WAP password changes.
        if (cJSON_IsString(item) && item->valuestring != NULL) {
            if (strcmp(key, "ssid") == 0) {
                station->setSSID(item->valuestring);
                arrSize = static_cast<int>(IDXSIZE::SSID);
            } else if (strcmp(key, "pass") == 0) {
                station->setPass(item->valuestring);
                arrSize = static_cast<int>(IDXSIZE::PASS);
            } else if (strcmp(key, "phone") == 0) {
                station->setPhone(item->valuestring);
                arrSize = static_cast<int>(IDXSIZE::PHONE);
            } else if (strcmp(key, "WAPpass") == 0) {
                wap->setPass(item->valuestring);
                arrSize = static_cast<int>(IDXSIZE::PASS);
            }

            // Will write to NVS as this method's primary objective.
            creds->write(key, item->valuestring, arrSize);
            strcpy(writtenKey, key);

            break;
        } 
    }

    cJSON_Delete(json);

    cJSON* response = cJSON_CreateObject();

    char respStr[35] = "Error";
    bool markForDest{false};

    // If the WAPpass is changed, this will trigger a destruction and 
    // re-initialization of the connection with the new password.
    if (strcmp(writtenKey, "WAPpass") == 0) {
        strcpy(respStr, "Accepted, Reconnect to WiFi");
        markForDest = true;
    } else if (strcmp(writtenKey, "NULL") == 0) {
        return ESP_FAIL;
    } else {
        strcpy(respStr, "Accepted");
    }

    cJSON_AddItemToObject(
            response, 
            "status", 
            cJSON_CreateString(respStr)
            );

    char* responseJSON = cJSON_Print(response);
    httpd_resp_send(req, responseJSON, HTTPD_RESP_USE_STRLEN);
    
    cJSON_Delete(response);
    free(responseJSON);

    if (markForDest) wap->destroy();

    return ESP_OK;
}

}