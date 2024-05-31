#ifndef ESPCOM_H
#define ESPCOM_H
#include <Wire.h>

// Values and commands, copied from nodeMCU arduinoCom.h
#define CHECK_ALL 0

#define TEMP_HUM 1 // Sensor
#define GET_TEMP_HUM 2

#define SOIL_1 3 // Sensor
#define GET_SOIL_1 4

#define ACTIVATE_RELAY_1 5
#define DEACTIVATE_RELAY_1 6

uint8_t address;
char reply[80];
void begin(uint8_t address);
void receive(int buffersize);
void send();


#endif // ESPCOM_H