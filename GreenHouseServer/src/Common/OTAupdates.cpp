#include "Common/OTAupdates.h"
#include <ArduinoOTA.h>
#include <WiFiUdp.h>

namespace UpdateSystem {

// The reference to the sensorThread is passed to suspend the thread during 
// OTA updates. Upon an OTA update, the OLED display override will be set
// to true to block the net display, until reset.
OTAupdates::OTAupdates(
    UI::Display &OLED, Threads::SensorThread &sensorThread) : 

    OLED{OLED}, sensorThread{sensorThread},
    hasStarted{false} {
    memset(buffer, 0, sizeof(buffer));
}

void OTAupdates::start(){

    // Initialize the handlers
    ArduinoOTA.onStart([this]() {
        this->OLED.setOverrideStat(true);
        this->sensorThread.suspendTask();

        // U_FLASH will be the primary means for both online and from you PC.
        // SPIFFS is the other and will be used when updatingthe File System.
        if (ArduinoOTA.getCommand() == U_FLASH) {
            strcpy(this->buffer, "Updating\nSketch"); // This will be the primary
        } else { // U_SPIFFS is updates to the File System
            strcpy(this->buffer, "Updating\nFile Sys");
        }
        
        this->OLED.printUpdates(this->buffer); delay(500); // delay to display
    });

    ArduinoOTA.onProgress([&](uint32_t progress, uint32_t total) {
        uint32_t percentage{(progress * 100) / total};
        sprintf(this->buffer, "%d%%", percentage);
        this->OLED.updateProgress(this->buffer);
    });

    ArduinoOTA.onEnd([&]() {
        strcpy(this->buffer, "Complete");
        this->OLED.printUpdates(this->buffer); 
        this->OLED.setOverrideStat(false);
        this->sensorThread.resumeTask(); // resumes thread tasks
    });

    ArduinoOTA.onError([&](ota_error_t error) {
        switch(error) {
            case OTA_AUTH_ERROR:
            strcpy(this->buffer, "Auth\nFailed"); break;

            case OTA_BEGIN_ERROR:
            strcpy(this->buffer, "Begin\nFailed"); break;

            case OTA_CONNECT_ERROR:
            strcpy(this->buffer, "Connect\nFailed"); break;

            case OTA_RECEIVE_ERROR:
            strcpy(this->buffer, "Receive\nFailed"); break;

            case OTA_END_ERROR:
            strcpy(this->buffer, "End\nFailed"); 
        }

        this->OLED.printUpdates(buffer);
        this->OLED.setOverrideStat(false);
;       this->sensorThread.resumeTask();; // resumes thread tasks
    });

    ArduinoOTA.begin();
}

void OTAupdates::handle(){
    ArduinoOTA.handle(); // Handle updates
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