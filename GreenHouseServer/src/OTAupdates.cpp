#include "OTAupdates.h"
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

namespace UpdateSystem {

// The reference to the sensorThread is passed to suspend the thread during 
// OTA updates
OTAupdates::OTAupdates(UI::Display &OLED, Threads::SensorThread &sensorThread) : 
OTAisUpdating(false), 
OLED(OLED), 
sensorThread(sensorThread),
hasStarted(false) {
    memset(buffer, 0, sizeof(buffer));
}

void OTAupdates::start(){

    // Initialize the handlers
    ArduinoOTA.onStart([this]() {
        this->OTAisUpdating = true;
        this->sensorThread.suspendTask(); // Stops thread to prevent interference
        
        // This will differentiate between updating on your pc, and firmware
        // udpates downloaded online.
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
        this->sensorThread.resumeTask();
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
        this->sensorThread.resumeTask();
    });

    ArduinoOTA.begin();
}

void OTAupdates::handle(){
    ArduinoOTA.handle(); // Handle updates
}

// Serves to block the typical OLED display during an update to show
// status.
bool OTAupdates::isUpdating() const{ 
    return OTAisUpdating;
}

// Serves to show that the OTA start has been called. This 
// happens only once.
bool OTAupdates::getHasStarted() {
    return this->hasStarted;
}

void OTAupdates::setHasStarted(bool value) {
    this->hasStarted = value;
}

void OTAupdates::manageOTA(Comms::Station &station) {

    // Safety that only allows the ota updates to begin when connected
    // to the station. Once started it doesn't stop until the end of the
    // program, but will not handle any updates unless station connected.
    if (station.isSTAconnected() && !this->getHasStarted()) {
        this->start();
        this->setHasStarted(true); 
    } else if(station.isSTAconnected() && this->getHasStarted()) {
        this->handle(); // checks for firmware 
    }
}
}