#include "Network.h"
#include <ArduinoJson.h>
#include "eeprom.h"

// This has to have all of the servers in here. The intention is to 
// setup all routes at once, to avoid redundancy, as well as begin the
// server to handle the client.

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
                String jsonData; jsonData.reserve(256);
                String ssid; ssid.reserve(32);
                String pass; pass.reserve(64);
                String phone; phone.reserve(15);
                String response; response.reserve(64);
                
                jsonData = server.arg("plain");
                DynamicJsonDocument jsonDoc(256);

                if (deserializeJson(jsonDoc, jsonData)) { // shows error if true
                    strcpy(this->error, "JSON deserialization ERR");
                    OLED.displayError(this->error); return; // prevents further issues
                };

                ssid = jsonDoc["ssid"].as<String>();
                pass = jsonDoc["pass"].as<String>();
                phone = jsonDoc["phone"].as<String>();

                STAsettings writeToEEPROM;
                DynamicJsonDocument res(64); // sends back to client

                // This is done during the initial save of the variables to EEPROM. This
                // exists to serve as a backup in the event EEPROM fails. If it does fail
                // The response to the client will be TScode:1, which means EEPROM fail. 
                // The EEPROM will fail when the read back != the input, but it will still 
                // allow the user to connect to the LAN. However, this is volatile memory 
                // and will need to be done upon every powerdown, but its doable while waiting
                // for a replacement. 
                strncpy(this->ST_SSID, ssid.c_str(), sizeof(this->ST_SSID) - 1);
                this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0';

                strncpy(this->ST_PASS, pass.c_str(), sizeof(this->ST_PASS) - 1);
                this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';

                strncpy(this->phoneNum, phone.c_str(), sizeof(this->phoneNum) - 1);
                this->phoneNum[sizeof(this->phoneNum) - 1] = '\0';

                if(writeToEEPROM.eepromWrite(ssid, pass, phone)) {
                    res["status"] = "Received, swap to station mode to check connection";
                } else {
                    res["status"] = "Error storing data, swap to station (TScode:1)";
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