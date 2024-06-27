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
            Messaging::Levels::ERROR,
            "JSON not serialize for web response",
            Messaging::Method::SRL
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
            Messaging::Levels::WARNING,
            "Passed JSON key that wasnt recognized",
            Messaging::Method::SRL); 
}

void WirelessAP::getJson(
    const char* key, char* jsonData, 
    JsonDocument &jsonDoc, NVS::Credentials &Creds) {

    char buffer[100]{};
    bool containsKey{false};

    strncpy(buffer, jsonDoc[key].as<const char*>(), sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0'; // ensure null termination for storage

    // Redundancy for NVS failire. Copies data from WAP submit directly to 
    // class variables. On startup, net variables are empty resulting in
    // NVS values. This loop checks each credential key against the passed 
    // key, and a match copies the json data to the corresponding class variable.
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
    char jsonData[100]{}; // 100 is padded, will never exceed 75
    char buffer[100]{};
    JsonDocument jsonDoc{};
    bool containsKey{false};

    // true = body contains json.
    if (NetMain::server.hasArg("plain")) {
        
        const char* plain{NetMain::server.arg("plain").c_str()};
        size_t plainLength{strlen(plain)};

        // Redundancy, should never overflow.
        if (plainLength < sizeof(jsonData)) {
            strncpy(jsonData, plain, plainLength);
            jsonData[plainLength] = '\0';
        } else {
            NetMain::server.send(413, "text/plain", "Payload too large");
        }

        DeserializationError error{deserializeJson(jsonDoc, jsonData)};

        // Halts function if error.
        if (error) { 
            this->msglogerr.handle(
                Messaging::Levels::ERROR,
                "JSON deserialization error submitting creds",
                Messaging::Method::OLED, Messaging::Method::SRL
            ); return; 
        };

        // Iterates the struct and if the incoming json key matches an expected
        // key, progresses.
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

// This controller handles and decodes JSON data in order to update both the
// class variables and NVS data.
void WirelessAP::handleWAPsubmit(NVS::Credentials &Creds) {

    if (NetMain::prevServerType == WIFI::WAP_SETUP) {
        WirelessAP::handleJson(Creds);
    } else {
        NetMain::server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
    }
};

}