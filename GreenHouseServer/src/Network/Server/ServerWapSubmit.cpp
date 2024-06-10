#include "Network.h"
#include <ArduinoJson.h>

namespace Comms {

// Writes to EEPROM and sends response to client. 
void Net::commitAndRespond(
    const char* type, 
    FlashWrite::Credentials &Creds, 
    char* buffer) {
    char response[64];
    JsonDocument res; // sends back to client

    if (Creds.write(type, buffer)) {
        // This looks to see if the WAP password has been changed. If so,
        // it sends a different response and resets the wifi below with 
        // the new password.
        if (strcmp(type, "WAPpass") == 0) {
            res["status"] = "Accepted, Reconnect to WiFi";
        } else {
            res["status"] = "Accepted";
        }

    } else {
        res["status"] = "Not Accepted, Error Storing Data";
    }

    serializeJson(res, response);
    server.send(200, "application/json", response);

    if (strcmp(type, "WAPpass") == 0) {
        WiFi.disconnect(); // Resets after setting WAP password
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_Pass, 1, 0, 1); // Allows 1 connection for security
    }
}

void Net::handleJson(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    // Never expect any single element coming through to exceed 75 char.
    char jsonData[100] = "";
    char buffer[100] = "";
    JsonDocument jsonDoc;

    auto getJson = [this, &buffer, &jsonDoc, &Creds](const char* type) {
        strncpy(
            buffer, 
            jsonDoc[type].as<const char*>(), 
            sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';

        // These strncpy's serve as a redundancy in the event the NVS fails.
        // This data when received from the WAP, is automatically copied to the 
        // required Network variables, which are ultimately used to begin the 
        // connection. If the Network variables are empty, it will default to 
        // checking the NVS, usually this happens upon starting.
        if (strcmp(type, "ssid") == 0) {
            strncpy(this->ST_SSID, buffer, sizeof(this->ST_SSID) - 1);
            this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0';
        } else if (strcmp(type, "pass") == 0) {
            strncpy(this->ST_PASS, buffer, sizeof(this->ST_PASS) - 1);
            this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';
        } else if (strcmp(type, "phone") == 0) {
            strncpy(this->phone, buffer, sizeof(this->phone) - 1);
            this->phone[sizeof(this->phone) - 1] = '\0';
        } else if (strcmp(type, "WAPpass") == 0) {
            strncpy(this->AP_Pass, buffer, sizeof(this->AP_Pass) - 1);
            this->AP_Pass[sizeof(this->AP_Pass) - 1] = '\0';
        }

        this->commitAndRespond(type, Creds, buffer);
    };

    if (server.hasArg("plain")) {
        
        const char* plain = server.arg("plain").c_str();
        size_t plainLength = strlen(plain);

        // redundancy, this should not every be an issue for buffer 
        // overflow
        if (plainLength < sizeof(jsonData)) {
            strncpy(jsonData, plain, plainLength);
            jsonData[plainLength] = '\0';
        } else {
            server.send(413, "text/plain", "Payload too large");
        }

        if (deserializeJson(jsonDoc, jsonData)) { // shows error if true
            strcpy(this->error, "JSON deserialization ERR");
            OLED.displayError(this->error); return; // prevents further issues
        };

        if (jsonDoc.containsKey("ssid")) getJson("ssid");
        if (jsonDoc.containsKey("pass")) getJson("pass");
        if (jsonDoc.containsKey("phone")) getJson("phone");
        if (jsonDoc.containsKey("WAPpass")) getJson("WAPpass");

    } else {
        server.send(400, "text/plain", "Bad Request. No JSON payload!");
    }
}

void Net::handleWAPsubmit(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    if (this->prevServerType == WAP_SETUP) {
        this->handleJson(OLED, Creds);
    } else {
        server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
    }
};
}