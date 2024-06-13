#include "Display.h"
#include <Wire.h>
#include <WiFi.h> // used for ESP.restart()

namespace UI {
// Constructor 
Display::Display(uint8_t width, uint8_t height) :
    display{width, height, &Wire, -1}, 
    displayOverride{false},
    width{width}, 
    height{height}{}

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
    display.display();
    delay(2000);
}

// updating == true will kill the display and allow the OLED to show the OTA updates
// being downloaded or uploaded to the device

// WAPtype will tell if it is exclusive WAP = 0, or WAPsetup = 1;
void Display::printWAP(
    const char SSID[11], 
    const char ipaddr[16], 
    const char status[4], 
    const char WAPtype[20], 
    bool updating,
    const char heap[10]) {
    if (updating == false && displayOverride == false) {
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
        display.display();
        } 
}

void Display::printSTA(
    Comms::STAdetails &details, 
    const char status[4], 
    bool updating,
    const char heap[10]) {
    if (updating == false && displayOverride == false) {
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
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(update);
    display.display();

}

// Handles the progress of the OTA update exclusively
void Display::updateProgress(char* progress) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("Progress");
    display.setCursor(0, 30);
    display.println(progress);
    display.display();
}

// displays errors if they occur during the normal functioning
void Display::displayError(char* error) {  // doesnt need to repeate overide, declaration only
    this->displayOverride = true;
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("Err/Msg: ");
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println(error);
    display.display();
}

bool Display::getOverrideStat() {
    return this->displayOverride;
}

void Display::setOverrideStat(bool setting) {
    this->displayOverride = setting;
}

}
