#include "Network.h"
#include <ArduinoJson.h>
#include "eeprom.h"

void Net::startServer(IDisplay &OLED) {
    auto handleIndex = [this]() {
        if (this->prevServerType == WAP_ONLY) {
            server.send(200, "text/html", "THIS IS WAP ONLY");
        } else if (this->prevServerType == WAP_SETUP) {
            server.send(200, "text/html", WAPsetup);
        } else if (this->prevServerType == STA_ONLY) {
            server.send(200, "text/html", "THIS IS STATION ONLY");
        }
    };

    // ADD ENCRYPTION AS WELL AS HTTPS FOR WAPSETUP SERVER
    auto handleWAPsubmit = [this, &OLED]() {
        if (this->prevServerType == WAP_SETUP) {
            if (server.hasArg("plain")) {
                char jsonData[100] = "";
                char buffer[100] = "";
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
                
                DynamicJsonDocument jsonDoc(100);

                STAsettings writeToEEPROM;
                DynamicJsonDocument res(64); // sends back to client
                char response[64];

                if (deserializeJson(jsonDoc, jsonData)) { // shows error if true
                    strcpy(this->error, "JSON deserialization ERR");
                    OLED.displayError(this->error); return; // prevents further issues
                };

                auto write = [&buffer, &writeToEEPROM, &res](const char* type){
                    if (writeToEEPROM.eepromWrite(type, buffer)) {
                        res["status"] = "Accepted";
                    } else {
                        res["status"] = "Not Accepted, Error Storing Data";
                    }
                };

                if (jsonDoc.containsKey("ssid")) {
                    strncpy(
                        buffer, 
                        jsonDoc["ssid"].as<const char*>(), 
                        sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    strncpy(this->ST_SSID, buffer, sizeof(this->ST_SSID) - 1);
                    this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0';
                    write("ssid");

                } else if (jsonDoc.containsKey("pass")) {
                    strncpy(
                        buffer, 
                        jsonDoc["pass"].as<const char*>(), 
                        sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    strncpy(this->ST_PASS, buffer, sizeof(this->ST_PASS) - 1);
                    this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';
                    write("pass");

                } else if (jsonDoc.containsKey("phone")) {
                    strncpy(
                        buffer, 
                        jsonDoc["phone"].as<const char*>(), 
                        sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    strncpy(this->phoneNum, buffer, sizeof(this->phoneNum) - 1);
                    this->phoneNum[sizeof(this->phoneNum) - 1] = '\0';
                    write("phone");

                } else if (jsonDoc.containsKey("WAPpass")) {
                    strncpy(
                        buffer, 
                        jsonDoc["WAPpass"].as<const char*>(), 
                        sizeof(buffer) - 1);
                    buffer[sizeof(buffer) - 1] = '\0';
                    strncpy(this->AP_Pass, buffer, sizeof(this->AP_Pass) - 1);
                    this->AP_Pass[sizeof(this->AP_Pass) - 1] = '\0';
                    write("WAPpass");
                }

                serializeJson(res, response);
                server.send(200, "application/json", response);
            } else {
                server.send(400, "text/plain", "Bad Request. No JSON payload!");
            }
        } else {
            server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
        }
    };

    auto handleNotFound = [this]() {
        server.send(404, "text/html", "Page Not Found");
    };

    server.on("/", handleIndex);
    server.on("/WAPsubmit", handleWAPsubmit);
    server.onNotFound(handleNotFound);
    server.begin(); 
    this->isServerRunning = true;
}

void Net::handleServer() {
    if (this->isServerRunning) { // begins after server.begin()
        this->server.handleClient();
    }
}

// Used to start the OTA updates
bool Net::isSTAconnected() {
    return this->connectedToSTA;
}
