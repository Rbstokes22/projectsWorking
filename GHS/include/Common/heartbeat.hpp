#ifndef HEARTBEAT_HPP
#define HEARTBEAT_HPP

#include "Threads/Mutex.hpp"
#include "UI/MsgLogHandler.hpp"

// ATTENTION. This is a software implementation of the watchdog timer used for
// troubleshooting and general performance. All calling functions will be able
// to set a 1 - 255 second timer. 

// ATTENTION. Suspensions are added to prevent issues with heartbeat failures
// during network scans, as well as other blocking functions such as client to
// server conenction open calls, that remain unresolved. Testing has shown
// blocking periods exceeding 7 seconds when URL/mDNS cannot be resolved, or
// hangs, resulting in devices restarting. Suspensions

namespace heartbeat {

#define HEARTBEAT_TAG "(Heartbeat)"
#define HEARTBEAT_CLIENTS_MAX 32 // Do not exceed 32, 
#define HEARTBEAT_CLIENTS_FULL 32
#define HEARTBEAT_CALLER_CHAR_LEN 16 // 16 chars max including null term.
#define HEARTBEAT_RESET_FAILS 3 // allowable consecutive HB fails before restart
#define HEARTBEAT_MAX_REASON 64 // bytes allowed for suspension reason
#define HEARTBEAT_DEFAULT_REASON "N/A" // used when reason exceeds limit
#define HEARTBEAT_REL_EXT 3 // Extends heartbeats after release to avoid err

class Heartbeat {
    private:
    const char* tag;
    static uint8_t blockNum; 
    char log[LOG_MAX_ENTRY];
    uint8_t reg[HEARTBEAT_CLIENTS_MAX]; // registry
    uint8_t failures[HEARTBEAT_CLIENTS_MAX]; // HB resp failures.
    uint32_t suspensionReg; // Tracks if heartbeat is suspended
    bool allSus; // Flag to indicate all heartbeats are suspended.
    char callers[HEARTBEAT_CLIENTS_MAX][16];
    static Threads::Mutex mtx; // mutex
    Heartbeat(const char* tag);
    Heartbeat(const Heartbeat&) = delete; // Prevent copying.
    Heartbeat &operator=(const Heartbeat&) = delete; // Prevent assignment.
    bool checkSus(uint8_t ID);
    void alert(uint8_t ID);

    public:
    static Heartbeat* get();
    uint8_t getBlockID(const char* caller, uint8_t initVal);
    void rogerUp(uint8_t chunkID, uint8_t resetSec);
    void manage();
    void suspend(uint8_t chunkID, const char* reason);
    void suspendAll(const char* reason);
    void release(uint8_t chunkID);
    void releaseAll();
};

}

#endif // HEARTBEAT_HPP