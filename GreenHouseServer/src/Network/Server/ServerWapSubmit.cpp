#include "Network.h"
#include <ArduinoJson.h>

// This source file handles the WAP submit JSON data that is passed
// from the webpage to setup the station network credentials as well
// as change the existing WAP password.

namespace Comms {

// Takes the buffer data and writes it to the NVS with a specific key
// value.
void WirelessAP::commitAndRespond(
    const char* type, FlashWrite::Credentials &Creds, char* buffer
    ) {

    char response[64]; // Will never exceed this limit.
    JsonDocument res; // this variable is sent back to client as response

    if (Creds.write(type, buffer)) {
        // If the WAP password has been changed, alerts client to reconnect
        // to the WiFi since it will disconnect.
        if (strcmp(type, "WAPpass") == 0) {
            res["status"] = "Accepted, Reconnect to WiFi";
        } else {
            res["status"] = "Accepted"; // type != WAPpass
        }

    } else {
        // Error upon writing to NVS.
        res["status"] = "Not Accepted, Error Storing Data";
    }

    // serialize JSON response and send it back to client.
    serializeJson(res, response);
    NetMain::server.send(200, "application/json", response);

    // Restarts the WiFi connection upon a password change. This is because
    // the change takes place immediately to prevent any sort of security
    // issue.
    if (strcmp(type, "WAPpass") == 0) {
        WiFi.disconnect(); // Resets after setting WAP password
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(this->local_IP, this->gateway, this->subnet);

        // Uses new pass setup in the handleJson method
        WiFi.softAP(this->AP_SSID, this->AP_PASS, 1, 0, 1); // Allows 1 connection for security
    }
}

void WirelessAP::handleJson(UI::IDisplay &OLED, FlashWrite::Credentials &Creds) {
    // The largest element coming though will be 64 char array with a 4 char 
    // key. Never expect it to exceed 75 bytes, 100 for padding.
    char jsonData[100] = "";
    char buffer[100] = "";
    JsonDocument jsonDoc;

    // The type passed to this will determine which class variable the 
    // data will be assigned to, and copy the buffer data over to it.
    // This is middleware before the commit to NVS and server response.
    auto getJson = [this, &buffer, &jsonDoc, &Creds](const char* type) {
        strncpy(
            buffer, jsonDoc[type].as<const char*>(), sizeof(buffer) - 1
            );

        buffer[sizeof(buffer) - 1] = '\0';

        // These strncpy's serve as a redundancy in the event the NVS fails.
        // This data when received from the WAP, is automatically copied to the 
        // required Network variables, which are ultimately used to begin the 
        // connection. If the Network variables are empty, it will default to 
        // checking the NVS, usually this happens upon starting.
        if (strcmp(type, "ssid") == 0) {
            strncpy(NetMain::ST_SSID, buffer, sizeof(NetMain::ST_SSID) - 1);
            NetMain::ST_SSID[sizeof(NetMain::ST_SSID) - 1] = '\0';

        } else if (strcmp(type, "pass") == 0) {
            strncpy(NetMain::ST_PASS, buffer, sizeof(NetMain::ST_PASS) - 1);
            NetMain::ST_PASS[sizeof(NetMain::ST_PASS) - 1] = '\0';

        } else if (strcmp(type, "phone") == 0) {
            strncpy(NetMain::phone, buffer, sizeof(NetMain::phone) - 1);
            NetMain::phone[sizeof(NetMain::phone) - 1] = '\0';

        } else if (strcmp(type, "WAPpass") == 0) {
            strncpy(this->AP_PASS, buffer, sizeof(this->AP_PASS) - 1);
            this->AP_PASS[sizeof(this->AP_PASS) - 1] = '\0';
        }

        // Commits to NVS and sends server response.
        this->commitAndRespond(type, Creds, buffer);
    };

    // If this argument exists, it means the body contains JSON. 
    if (NetMain::server.hasArg("plain")) {
        
        const char* plain = NetMain::server.arg("plain").c_str();
        size_t plainLength = strlen(plain);

        // Redundancy, this should not ever be an issue for buffer 
        // overflow.
        if (plainLength < sizeof(jsonData)) {
            strncpy(jsonData, plain, plainLength);
            jsonData[plainLength] = '\0';
        } else {
            NetMain::server.send(413, "text/plain", "Payload too large");
        }

        DeserializationError error = deserializeJson(jsonDoc, jsonData);

        // JSON is deserialized and ready for use.
        if (error) { 
            strcpy(NetMain::error, "JSON deserialization ERR");
            OLED.displayError(NetMain::error); return; 
        };

        // Depending on the json key passed, this will get the data 
        // associated with that key. Only one key:val will be passed
        // at a time.
        if (jsonDoc.containsKey("ssid")) getJson("ssid");
        if (jsonDoc.containsKey("pass")) getJson("pass");
        if (jsonDoc.containsKey("phone")) getJson("phone");
        if (jsonDoc.containsKey("WAPpass")) getJson("WAPpass");

    } else {
        NetMain::server.send(400, "text/plain", "Bad Request. No JSON payload!");
    }
}

// Upon being called from the client, this controller will handle and decode 
// the JSON data sent through in order to store it in NVS and change the 
// class variables.
void WirelessAP::handleWAPsubmit(
    UI::IDisplay &OLED, FlashWrite::Credentials &Creds
    ) {

    if (NetMain::prevServerType == WAP_SETUP) {
        WirelessAP::handleJson(OLED, Creds);
    } else {
        NetMain::server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
    }
};

}