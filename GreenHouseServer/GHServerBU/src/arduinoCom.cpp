#include "arduinoCom.h"
#include <Arduino.h>

I2Ccomm::I2Ccomm(uint8_t address) : address(address){}

void I2Ccomm::begin() {
    Wire.begin();
    Wire.setClock(400000); // sets to 400khz to meet nano's max
    
}

void I2Ccomm::send(uint8_t command, int nextUpdate) {
    char buffer[32];
    snprintf(buffer, 32, "%d/%d", command, nextUpdate);
    Wire.beginTransmission(this->address);
    Wire.write(buffer); 
    Wire.endTransmission();
}

void I2Ccomm::receive(char* buffer, size_t bufferSize) {
    int ival = 0;
    Wire.requestFrom(this->address, bufferSize - 1); 
    while(Wire.available() && ival < bufferSize - 1) { // prevents overflow
        buffer[ival++] = Wire.read();
    }
    buffer[ival] = '\0'; // Terminate the string
}