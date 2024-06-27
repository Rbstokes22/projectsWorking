#include "UI/Display.h"
#include <Wire.h>
#include <WiFi.h> 

// The OLED's primary duty is to display the Net data. It will 
// also display the OTA updating progress as well as any urgent
// runtime errors or messages. These are mutually exclusive and 
// will not be handled at the same time. Net data will only 
// be displayed if there is no override or ota update happening.

namespace UI {

// Constructor 
Display::Display(uint8_t width, uint8_t height) :

    display{width, height, &Wire, -1}, 
    displayOverride{false},
    msgAddresses{0},
    lastIndex{0},
    width{width}, 
    height{height}{memset(this->msgBuffer, 0, sizeof(this->msgBuffer));}

// Initialization and startup
void Display::init() {
    displayStatus = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (!displayStatus) { 
        ESP.restart(); // restarts ESP if display is not initializing
    }

    // Hard coded names during init.
    display.clearDisplay();
    display.drawBitmap(0, 0, GHlogo, width, height, WHITE);
    display.display();
    delay(2000);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("SSTech ");
    display.setTextSize(1);
    display.println("2024");
    display.setCursor(0, 30);
    display.println("Mumsy's Greenhouse");
    display.display();
    delay(2000);
}

void Display::printWAP(
    const char SSID[11], 
    const char ipaddr[16], 
    const char status[4], 
    const char WAPtype[20], 
    const char heap[10],
    const uint8_t clientsConnected) {
    if (!this->displayOverride) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0,0);
        display.print("Broadcasting: ");
        display.println(status);
        display.setCursor(0, 10);
        display.print("SSID: ");
        display.println(SSID);
        display.setCursor(0, 20);
        display.print("IP: ");
        display.println(ipaddr);
        display.setCursor(0, 30);
        display.println(WAPtype);
        display.setCursor(0, 40);
        display.print("Free Mem: ");
        display.println(heap);
        display.setCursor(0, 50);
        display.print("Connected #: ");
        display.println(clientsConnected);
        display.display();
    } 
}

void Display::printSTA(
    Comms::STAdetails &details, 
    const char status[4], 
    const char heap[10]) {
    if (!this->displayOverride) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("SSID/Network:");
        display.setCursor(0, 10);
        display.println(details.SSID);
        display.setCursor(0, 20);
        display.print("IP: ");
        display.println(details.IPADDR);
        display.setCursor(0, 30);
        display.print("Connected: ");
        display.println(status);
        display.setCursor(0, 40);
        display.print("Sig: ");
        display.println(details.signalStrength);
        display.setCursor(0, 50);
        display.print("Free Mem: ");
        display.println(heap);
        display.display();
    }
}

// Handles regular OTA updates status changes
void Display::printUpdates(char* update) {
    if (this->displayOverride) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(0, 0);
        display.println(update);
        display.display();
    }
}

// Handles the progress of the OTA update exclusively
void Display::updateProgress(char* progress) {
    if (this->displayOverride) {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(0,0);
        display.println("Progress");
        display.setCursor(0, 30);
        display.println(progress);
        display.display();
    }
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
// The new beginning address 
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

    this->displayOverride = true; // block normal network status display.
    appendMessage(msg);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(this->msgBuffer);
    display.display();
}

bool Display::getOverrideStat() {
    return this->displayOverride;
}

void Display::setOverrideStat(bool setting) {
    this->displayOverride = setting;
}

}
