#include "Network/NetworkWAP.h"
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
        
    char response[64]{}; // Will never exceed this limit.
    JsonDocument res{}; // this variable is sent back to client as response

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
    size_t bytesWritten = serializeJson(res, response);
    if (bytesWritten == 0) {
        this->msglogerr.handle(
            Levels::ERROR,
            "JSON not serialize for web response",
            Method::SRL
        );
        strcpy(response, "{'status':'Error JSON serialization'}");
    }

    NetMain::server.send(200, "application/json", response);

    // Restarts the WiFi connection upon a password change. This is because
    // the change takes place immediately to prevent any sort of security
    // issue. If issue, toggle switch should clear up, but look into errors
    // during testing.
    if (strcmp(type, "WAPpass") == 0) {
        this->WAPconnect(1); // 1 client for security reasons
    }
}

void WirelessAP::handleJson(FlashWrite::Credentials &Creds) {
    // The largest element coming though will be 64 char array with a 4 char 
    // key. Never expect it to exceed 75 bytes, 100 for padding.
    char jsonData[100]{};
    char buffer[100]{};
    JsonDocument jsonDoc{};

    // The type passed to this will determine which class variable the 
    // data will be assigned to, and copy the buffer data over to it.
    // This is middleware before the commit to NVS and server response.
    auto getJson = [this, &buffer, &jsonDoc, &Creds](const char* type) {
        strncpy(
            buffer, jsonDoc[type].as<const char*>(), sizeof(buffer) - 1
            );

        buffer[sizeof(buffer) - 1] = '\0'; // ensure null termination for storage

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
        
        const char* plain{NetMain::server.arg("plain").c_str()};
        size_t plainLength{strlen(plain)};

        // Redundancy, this should not ever be an issue for buffer 
        // overflow.
        if (plainLength < sizeof(jsonData)) {
            strncpy(jsonData, plain, plainLength);
            jsonData[plainLength] = '\0';
        } else {
            NetMain::server.send(413, "text/plain", "Payload too large");
        }

        DeserializationError error{deserializeJson(jsonDoc, jsonData)};

        // JSON is deserialized and ready for use.
        if (error) { 
            this->msglogerr.handle(
                Levels::ERROR,
                "JSON deserialization error submitting creds",
                Method::OLED, Method::SRL
            ); return; 
        };

        // Depending on the json key passed, this will get the data 
        // associated with that key. Only one key:val will be passed
        // at a time.
        if (jsonDoc.containsKey("ssid")) {getJson("ssid");}
        else if (jsonDoc.containsKey("pass")) {getJson("pass");}
        else if (jsonDoc.containsKey("phone")) {getJson("phone");}
        else if (jsonDoc.containsKey("WAPpass")) {getJson("WAPpass");}
        else {
            this->msglogerr.handle(
                Levels::WARNING,
                "Passed JSON key that wasnt recognized",
                Method::SRL
            );
        }

    } else {
        NetMain::server.send(400, "text/plain", "Bad Request. No JSON payload!");
    }
}

// Upon being called from the client, this controller will handle and decode 
// the JSON data sent through in order to store it in NVS and change the 
// class variables.
void WirelessAP::handleWAPsubmit(FlashWrite::Credentials &Creds) {

    if (NetMain::prevServerType == WIFI::WAP_SETUP) {
        WirelessAP::handleJson(Creds);
    } else {
        NetMain::server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
    }
};

}