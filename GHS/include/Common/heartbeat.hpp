#ifndef HEARTBEAT_HPP
#define HEARTBEAT_HPP

#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

// ATTENTION. This is a software implementation of the watchdog timer used for
// troubleshooting and general performance. All calling functions will be able
// to set a 1 - 255 second timer. 

namespace heartbeat {

#define HEARTBEAT_TAG "(Heartbeat)"
#define HEARTBEAT_CLIENTS_MAX 32 // Max number of clients, do not exceed 255.
#define HEARTBEAT_CLIENTS_FULL 255;
#define HEARTBEAT_CALLER_CHAR_LEN 16 // 16 chars max including null term.

class Heartbeat {
    private:
    const char* tag;
    static uint8_t blockNum; 
    char log[LOG_MAX_ENTRY];
    uint8_t reg[HEARTBEAT_CLIENTS_MAX]; 
    char callers[HEARTBEAT_CLIENTS_MAX][16];
    static Threads::Mutex mtx; // mutex
    Heartbeat(const char* tag);
    Heartbeat(const Heartbeat&) = delete; // Prevent copying.
    Heartbeat &operator=(const Heartbeat&) = delete; // Prevent assignment.
    void alert(uint8_t ID);

    public:
    static Heartbeat* get();
    uint8_t getBlockID(const char* caller, uint8_t initVal);
    void rogerUp(uint8_t chunkID, uint8_t resetSec);
    void manage();
};

}

#endif // HEARTBEAT_HPP