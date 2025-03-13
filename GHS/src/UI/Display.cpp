#include "UI/Display.hpp"
#include <cstdint>
#include <cstddef>
#include "Drivers/SSD1306_Library.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// The OLED's primary duty is to display the Net data. It will 
// also display the OTA updating progress as well as any urgent
// runtime errors or messages. These are mutually exclusive and 
// will not be handled at the same time. Net data will only 
// be displayed if there is no override or ota update happening.

// Left in append and remove message methods. These were built to show 
// data on the OLED screen before the implementation of the log. These will
// remain in place to be used if desired in the future for critical updates
// that might not be accessable via wifi to the client in the form of a log.

namespace UI {

// Constructor 
Display::Display() : displayOverride{false} {}

// Initialization and startup
void Display::init(uint8_t address) {
    this->display.init(address);
    this->display.setCharDim(OLED_CHAR_SIZE);
    this->display.setPOS(0, 0);
    this->display.write(OLED_COMPANY_NAME);
    this->display.setPOS(0, 2);
    this->display.write(OLED_DEVICE_NAME);
    this->display.send();
    vTaskDelay(pdMS_TO_TICKS(3000));
}

void Display::printWAP(Comms::WAPdetails &details) {
    if (!this->displayOverride) {
        this->display.write("Broadcasting: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.status, UI_DRVR::TXTCMD::END);
        this->display.write(details.ipaddr);
        this->display.write(details.mdns);
        this->display.write(details.WAPtype);
        this->display.write("Free Mem: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.heap, UI_DRVR::TXTCMD::END);
        this->display.write("Connected #: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.clientConnected, UI_DRVR::TXTCMD::END);
        this->display.send();
    } 
}

void Display::printSTA(Comms::STAdetails &details) {
    if (!this->displayOverride) {
        this->display.write("SSID/NETWORK: ");
        this->display.write(details.ssid);
        this->display.write(details.ipaddr);
        this->display.write(details.mdns);
        this->display.write("Connected: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.status, UI_DRVR::TXTCMD::END);
        this->display.write("Sig: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.signalStrength, UI_DRVR::TXTCMD::END);
        this->display.write("Free Mem: ", UI_DRVR::TXTCMD::START);
        this->display.write(details.heap, UI_DRVR::TXTCMD::END);
        this->display.send();
    }
}

// Requires update message. This is a blocking function and should be called
// when necessary, meaning not during an OTA update, but between updates.
void Display::printUpdates(const char* update) {
    if (this->displayOverride) {
        this->display.write(update);
        this->display.send();
        // Delays for readability
        vTaskDelay(pdMS_TO_TICKS(OLED_UPDATE_DELAY_ms)); 
    }
}

// Handles the progress of the OTA update exclusively
void Display::updateProgress(const char* progress) {
    if (this->displayOverride) {
        this->display.write("OTA PROGRESS:", UI_DRVR::TXTCMD::START);
        this->display.write(progress, UI_DRVR::TXTCMD::END);
        this->display.send();
    }
}

// Used to handle if the boot firmware is invalid. Will display error
// message and not proceed. Used only for this.
void Display::invalidFirmware() {
    this->display.cleanWrite("CRITICAL: Corrupt Firmware");
    this->display.send();
}

// Requires message. This is best handled with the message, log, error handler
// since it will ensure that the overRide will expire. This is a non-blocking
// version of printUpdates() and will only clear once the message expires
// from the queue.
void Display::displayMsg(char* msg) {  // Override handled in msglogerr.
    if (msg == nullptr || *msg == '\0') return; // handles erronius args.
    if (strlen(msg) >= OLED_CAPACITY) return; // Prevents overflow

    this->display.setCharDim(UI_DRVR::DIM::D5x7); // Force if not set.
    this->display.write(msg);
    this->display.send();
}

bool Display::getOverrideStat() {
    return this->displayOverride;
}

void Display::setOverrideStat(bool setting) {
    this->displayOverride = setting;
}

size_t Display::getOLEDCapacity() const {
    return this->display.getOLEDCapacity();
}

}