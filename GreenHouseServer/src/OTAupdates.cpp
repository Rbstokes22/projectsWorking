#include "OTAupdates.h"
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

// Send OLED reference to update the 
OTAupdates::OTAupdates(Display &OLED) : 
OTAisUpdating(false), OLED(OLED), hasStarted(false){
    memset(buffer, 0, sizeof(buffer));
}

void OTAupdates::start(){

    // Initialize the handlers
    ArduinoOTA.onStart([&]() {
        this->OTAisUpdating = true;

        if (ArduinoOTA.getCommand() == U_FLASH) {
            strcpy(this->buffer, "updating sketch");
        } else { // U_SPIFFS to store in filesystem
            strcpy(this->buffer, "updating filesys");
        }
        
        OLED.printUpdates(this->buffer);
    });

    // use sprint f for the integers into the char array
    ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
        unsigned int percentage = (progress * 100) / total;
        sprintf(this->buffer, "%d%%", percentage);
        OLED.updateProgress(this->buffer);
    });

    ArduinoOTA.onEnd([&]() {
        // delay for viewing only
        strcpy(this->buffer, "Complete");
        OLED.printUpdates(this->buffer); 
        this->OTAisUpdating = false;
    });

    ArduinoOTA.onError([&](ota_error_t error) {
        switch(error) {
            case OTA_AUTH_ERROR:
            strcpy(this->buffer, "Auth Failed"); break;

            case OTA_BEGIN_ERROR:
            strcpy(this->buffer, "Begin Failed"); break;

            case OTA_CONNECT_ERROR:
            strcpy(this->buffer, "Connect Failed"); break;

            case OTA_RECEIVE_ERROR:
            strcpy(this->buffer, "Receive Failed"); break;

            case OTA_END_ERROR:
            strcpy(this->buffer, "End Failed"); 
        }

        OLED.printUpdates(buffer);
        this->OTAisUpdating = false;
    });

    ArduinoOTA.begin();
}

void OTAupdates::handle(){
    ArduinoOTA.handle(); // Handle updates
}

bool OTAupdates::isUpdating() const{ 
    return OTAisUpdating;
}

bool OTAupdates::getHasStarted() {
    return this->hasStarted;
}

void OTAupdates::setHasStarted(bool value) {
    this->hasStarted = value;
}