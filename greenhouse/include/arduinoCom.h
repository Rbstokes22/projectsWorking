#ifndef ARDUINOCOM_H
#define ARDUINOCOM_H
#include <Wire.h>

// Values and commands, copy and paste these into arduino slave
#define CHECK_ALL 0

#define TEMP_HUM 1
#define GET_TEMP_HUM 2

#define SOIL_1 3
#define GET_SOIL_1 4

#define ACTIVATE_RELAY_1 5
#define DEACTIVATE_RELAY_1 6

class I2Ccomm {
    private:
    uint8_t address;

    public:
    I2Ccomm(uint8_t address);
    void begin();
    void send(uint8_t command, int nextUpdate);
    void receive(char* buffer, size_t bufferSize);
};



#endif // ARDUINOCOM_H