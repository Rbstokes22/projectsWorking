#include "UI/Display.hpp"
#include <cstdint>
#include "UI/IDisplay.hpp"
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

namespace UI {

// Constructor 
Display::Display() :
 
    displayOverride{false},
    msgAddresses{0},
    lastIndex{0} {
        memset(this->msgBuffer, 0, sizeof(this->msgBuffer));
    }

// Initialization and startup
void Display::init(uint8_t address) {
    this->display.init(address);
    this->display.setCharDim(UI_DRVR::DIM::D5x7);
    this->display.setPOS(0, 0);
    this->display.write("SSTech 2024");
    this->display.setPOS(0, 2);
    this->display.write("Mumsy's Greenhouse");
    this->display.send();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
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

// Handles regular OTA updates status changes
void Display::printUpdates(const char* update) {
    if (this->displayOverride) {
        this->display.write(update);
        this->display.send();
        // Delays for readability
        vTaskDelay(500 / portTICK_PERIOD_MS); 
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
// message and not proceed.
void Display::invalidFirmware() {
    this->display.cleanWrite("CRITICAL: Corrupt Firmware");
    this->display.send();
}

// If the max capacity of the OLED at size 1 (168) is going to be exceeded
// by the next message, this removes the first message. This gets the starting
// address of the 2nd error (start), and calculates the remaining data (end).
// That block of memory is moved to address 0. The remaining portion of the 
// block is nullified. The message addresses array hold the beginning of each
// message. Upon remove, each address will be adjusted in position and to its
// appropriate starting address after the removal of message 1, and adjust the
// last index to the appropraite index.
void Display::removeMessage() {
    uint8_t start = msgAddresses[1];
    uint8_t end = static_cast<int>(UIvals::OLEDCapacity) - start;
    uint8_t numIndicies = sizeof(this->msgAddresses) / sizeof(this->msgAddresses[0]);

    memmove(this->msgBuffer, this->msgBuffer + start, end);
    memset(this->msgBuffer + end, 0, static_cast<int>(UIvals::OLEDCapacity) - end);

    uint8_t removedMsgSize = msgAddresses[1] - msgAddresses[0];
    for (int i = 1; i < numIndicies; i++) {
        msgAddresses[i - 1] = msgAddresses[i] - removedMsgSize;
    }

    this->lastIndex--;
}

// All messages are passed here for processing. If the message size is larger 
// than the remining buffer space, the messages are removed from the msgBuffer.
// This is displayed with a delimiter for separation to ensure max screen use
// to see previous messages.
void Display::appendMessage(char* msg) {

    char delimeter[] = ";     "; // separates the errors
    char tempBuffer[static_cast<int>(UIvals::OLEDCapacity)]{'\0'};
    size_t remaining = sizeof(this->msgBuffer) - strlen(this->msgBuffer);
    uint16_t messageSize = strlen(msg) + strlen(delimeter);
    uint8_t indicyCap = static_cast<int>(UIvals::msgIndicyTotal) - 2; // prevents overflow

    // Removes the first message and adjusts the remaining size. If the size is 
    // still non-compliant, The while loop will continue to execute. If message
    // exceeds limits, the error is handled in MsgLogHandler.
    auto manageRemoval = [this](size_t &remaining){
        this->removeMessage();
        remaining = sizeof(this->msgBuffer) - strlen(this->msgBuffer);
    };

    while(messageSize > remaining || lastIndex == indicyCap) {
        manageRemoval(remaining);
    }

    // Gets the string length of the buffer. If the length is 30,
    // this means that the concat message will begin at index 30,
    // and the block preceeding it is from 0 - 29 index value. 
    // The last index is then incrememented to aid in the next append.
    this->msgAddresses[this->lastIndex] = strlen(this->msgBuffer);
    this->lastIndex++;
 
    // appends the delimiter to the message and sends it to a temp buffer
    // to be concat to the message buffer.
    sprintf(tempBuffer, "%s%s", msg, delimeter); 
    strncat(this->msgBuffer, tempBuffer, remaining);
}

// Displays the messages and errors by appending them to a 
// message buffer. 
void Display::displayMsg(char* msg) {  
    if (msg == nullptr || *msg == '\0') return; // handles erronius args.

    this->display.setCharDim(UI_DRVR::DIM::D5x7);

    this->displayOverride = true; // block normal network status display.
    appendMessage(msg);
    this->display.write(this->msgBuffer);
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