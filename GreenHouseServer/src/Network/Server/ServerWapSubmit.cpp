#include "Network/NetworkWAP.h"

// This source file handles the WAP submit JSON data that is passed
// from the webpage to setup the station network credentials as well
// as change the existing WAP password.

namespace Comms {

// Takes the buffer data and writes it to the NVS with a specific key
// value.
void WirelessAP::commitAndRespond(
    const char* key, NVS::Credentials &Creds, char* buffer) {
        
    char response[64]{}; // Will never exceed this limit.
    JsonDocument res{}; // this variable is sent back to client as response

    if (Creds.write(key, buffer)) {
        // If the WAP password has been changed, alerts client to reconnect
        // to the WiFi since it will disconnect.
        if (strcmp(key, "WAPpass") == 0) {
            res["status"] = "Accepted, Reconnect to WiFi";
        } else {
            res["status"] = "Accepted"; // key != WAPpass
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
    if (strcmp(key, "WAPpass") == 0) {
        this->WAPconnect(1); // 1 client for security reasons
    }
}

// If no JSON key is found, this will send a Warning.
void WirelessAP::keyNotFound() {
    this->msglogerr.handle(
            Levels::WARNING,
            "Passed JSON key that wasnt recognized",
            Method::SRL); 
}

void WirelessAP::getJson(
    const char* key, char* jsonData, 
    JsonDocument &jsonDoc, NVS::Credentials &Creds) {

    char buffer[100]{};
    bool containsKey{false};

    strncpy(buffer, jsonDoc[key].as<const char*>(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; // ensure null termination for storage

    // These strncpy's serve as a redundancy in the event the NVS fails.
    // This data when received from the WAP, is automatically copied to the 
    // required Network variables, which are ultimately used to begin the 
    // connection. If the Network variables are empty, it will default to 
    // checking the NVS, usually this happens upon starting. This iterates
    // the credinfo array and if the passed key matches a required key,
    // its data is copied to the the class variable.
    for (auto &info : credinfo) {
        if (strcmp(key, info.key) == 0) {
            strncpy(info.destination, buffer, info.size - 1);
            info.destination[info.size - 1] = '\0';
            containsKey = true; break;
        }; 
    }

    if (!containsKey) {keyNotFound(); return;}
    
    // Commits to NVS and sends server response.
    this->commitAndRespond(key, Creds, buffer);
}

void WirelessAP::handleJson(NVS::Credentials &Creds) {
    // The largest element coming though will be 64 char array with a 4 char 
    // key. Never expect it to exceed 75 bytes, 100 for padding.
    char jsonData[100]{};
    char buffer[100]{};
    JsonDocument jsonDoc{};
    bool containsKey{false};

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

        // JSON is deserialized and ready for use, unless error. Then a message
        // is sent before the return.
        if (error) { 
            this->msglogerr.handle(
                Levels::ERROR,
                "JSON deserialization error submitting creds",
                Method::OLED, Method::SRL
            ); return; 
        };

        // MAYBE DO SOME ORGANIZING IN HERE
        for (auto &info : credinfo) {
            if (jsonDoc.containsKey(info.key)) {
                this->getJson(info.key, jsonData, jsonDoc, Creds);
                containsKey = true; break;
            }
        }

        if (!containsKey) {keyNotFound(); return;}

    } else {
        NetMain::server.send(400, "text/plain", "Bad Request. No JSON payload!");
    }
}

// Upon being called from the client, this controller will handle and decode 
// the JSON data sent through in order to store it in NVS and change the 
// class variables.
void WirelessAP::handleWAPsubmit(NVS::Credentials &Creds) {

    if (NetMain::prevServerType == WIFI::WAP_SETUP) {
        WirelessAP::handleJson(Creds);
    } else {
        NetMain::server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
    }
};

}