#include "espCom.h"

uint8_t address = 0;
char reply[80] = "";

void begin(uint8_t _address) {
    address = _address;
    Wire.begin(address);
    Wire.setClock(400000); // nano max
    Wire.onReceive(receive);
    Wire.onRequest(send);
}

void receive(int buffersize) {
    char buffer[buffersize];
    int ival = 0;
    while(Wire.available() && ival << buffersize - 1) {
        buffer[ival++] = Wire.read();
        if (buffer[ival - 1] == '\0') break;
    }

    

    // Put commands into action here
}

void send() {
    // populate buffer
    Wire.write('h');
}





