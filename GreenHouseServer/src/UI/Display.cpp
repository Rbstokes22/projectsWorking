#include "UI/Display.h"
#include <Wire.h>
#include <WiFi.h> // used for ESP.restart()

namespace UI {

// Constructor 
Display::Display(uint8_t width, uint8_t height) :

    display{width, height, &Wire, -1}, 
    displayOverride{false},
    msgIndicies{0},
    lastIndex{0},
    width{width}, 
    height{height}{memset(this->msgBuffer, 0, sizeof(this->msgBuffer));}

// Initialization and startup
void Display::init() {
    displayStatus = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (!displayStatus) { 
        ESP.restart(); // restarts ESP if display is not initializing
    }

    // If initialization is good, starts as normal
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

// The OLED's primary duty is to display the Net data. It will 
// also display the OTA updating progress as well as any urgent
// runtime errors or messages. These are mutually exclusive and 
// will not be handled at the same time. Net data will only 
// be displayed if there is no override or ota update happening.

// WAPtype will tell if it is exclusive WAP = 0, or WAPsetup = 1;
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

void Display::removeMessage() {
    uint8_t start = msgIndicies[1]; // beginning of the first msg after removal
    uint8_t end = static_cast<int>(UIvals::OLEDCapacity) - start;
    uint8_t numIndicies = sizeof(this->msgIndicies) / sizeof(this->msgIndicies[0]);

    // This will move the data from the msgBuffer that we care about down to index
    // 0. The rest of the data is untouched, so to nullify it, we will memset it 
    // from where the good data ends, for the remaining bytes of the block.
    memmove(this->msgBuffer, this->msgBuffer + start, end);
    memset(this->msgBuffer + end, 0, static_cast<int>(UIvals::OLEDCapacity) - end);

    // after deleting the first message, takes the starting address 
    // of the next message and shifts it down 1 index value in the 
    // msgIndicies array. This is because the first one is always 
    // stripped.
    uint8_t removedMsgSize = msgIndicies[1] - msgIndicies[0];
    for (int i = 1; i < numIndicies; i++) {
        msgIndicies[i - 1] = msgIndicies[i] - removedMsgSize;
    }

    this->lastIndex--;
}

// All error handling of messages that exceed the buffer length are handled in the MsgLogErr
void Display::appendMessage(char* msg) {
    char delimeter[] = ";     "; // separates the errors
    char tempBuffer[static_cast<int>(UIvals::OLEDCapacity)]{'\0'};
    size_t remaining = sizeof(this->msgBuffer) - strlen(this->msgBuffer);
    uint16_t messageSize = strlen(msg) + strlen(delimeter);
    uint8_t indicyCap = static_cast<int>(UIvals::msgIndicyTotal) - 2; // prevents overflow

    auto manageRemoval = [this](size_t &remaining){
        this->removeMessage();
        remaining = sizeof(this->msgBuffer) - strlen(this->msgBuffer);
    };

    while(messageSize > remaining || lastIndex == indicyCap) {
        manageRemoval(remaining);
    }

    Serial.println(this->msgBuffer);

    this->msgIndicies[this->lastIndex] = strlen(this->msgBuffer);
    this->lastIndex++;
 
    // appends the delimiter to the message and sends it to a temp buffer
    // to be concat to the message buffer.
    sprintf(tempBuffer, "%s%s", msg, delimeter); 
    strncat(this->msgBuffer, tempBuffer, remaining);
}

// displays err/msg if they occur during the normal functioning.
// The reason that there isnt a check if this->displayOverride is 
// set to true here, but nowhere else, is because the OTA updates
// set it to true in the OTA source file. This will be reset to 
// false when specified by the MsgLogHandler.
void Display::displayMsg(char* msg) {  
    if (msg == nullptr || *msg == '\0') return;

    this->displayOverride = true;
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
