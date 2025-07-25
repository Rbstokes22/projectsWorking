#ifndef RELAY_HPP
#define RELAY_HPP

#include "driver/gpio.h"
#include "Threads/Mutex.hpp"
#include <cstdint>
#include "UI/MsgLogHandler.hpp"

namespace Peripheral {

// Specifies the max amount of devices that can control single relay.
#define RELAY_IDS 10 
#define RELAY_TIMER_OFF 99999 // VAL means off
#define RELAY_DAYS 0b00000000 // Default no days set for timer.
#define RELAY_TAG_SIZE 16 
#define RELAY_NO_ID 255 // ID set with the number will not be usable.
#define RELAY_ID_STATES 4 // ID states in class enum
#define RELAY_ID_CALLER_LEN 16 // Chars allowable to caller tag.
#define RELAY_LOG_METHOD Messaging::Method::SRL_LOG

// Tags used for manual control. Will be used to send the client information if
// the relay is manually energized.
#define RELAY_MAN_0_TAG "SKTHANDRE0"
#define RELAY_MAN_1_TAG "SKTHANDRE1"
#define RELAY_MAN_2_TAG "SKTHANDRE2"
#define RELAY_MAN_3_TAG "SKTHANDRE3"


enum class RECOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Relay condition

// Relay phsyical state. Different from ID state, only one of these exist.
enum class RESTATE : uint8_t {
    OFF, // Relay is current off
    ON, // Relay is currently on
    FORCED_OFF, // Relay is being forced off, despite active clients using.
    FORCE_REMOVED // Remove the FORCE_OFF block.
};

// Client ID availability/state. Several controlling IDs per relay.
enum class IDSTATE : uint8_t {
    AVAILABLE, // ID Spot is available to be claimed.
    RESERVED, // ID Spot is reserved, but hasnt been used yet.
    ON, // ID is attempting to energize the relay.
    OFF // Id is relenquishing control over the relay.
};

struct Timer {
    uint32_t onTime; // time to turn relay on, seconds past midnight.
    uint32_t offTime; // time to turn relay off, secons past midnight.
    uint8_t days; // Days that the timer will be enabled. Stored as bitwise.
    bool isReady; // bot on and off times have been set and are not equal.
};

class Relay {
    private:
    char tag[RELAY_TAG_SIZE]; // Changes on relay number
    static char log[LOG_MAX_ENTRY]; // Shared between several objects.
    static const char* IDSTATEMap[RELAY_ID_STATES];
    uint8_t timerID; // Used exclusively for the relay timer.
    gpio_num_t pin; // Relay pin
    uint8_t ReNum; // Relay num, used as an ID. Not currently used thru program.
    RESTATE relayState; // Current relay state, 
    IDSTATE clients[RELAY_IDS]; // Client IDs attached to relay.
    char clientStr[RELAY_IDS][RELAY_ID_CALLER_LEN]; // client callers.
    uint8_t clientQty; // Clients currently energizing relay.
    Timer timer; // Timer to control relay on and off setings.
    Threads::Mutex mtx; // Mutex
    bool isAttached(uint8_t ID);
    bool changeIDState(uint8_t ID, IDSTATE newState);
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);
    const char* createTag(uint8_t ReNum);

    public:
    Relay(gpio_num_t pin, uint8_t ReNum);
    bool on(uint8_t ID);
    bool off(uint8_t ID);
    void forceOff();
    void removeForce();
    uint8_t getID(const char* caller);
    bool removeID(uint8_t ID);
    RESTATE getState();
    uint8_t getQty();
    bool isManual();
    bool timerSet(uint32_t onSeconds, uint32_t offSeconds);
    bool timerSetDays(uint8_t bitwise);
    void manageTimer();
    Timer* getTimer();
};

}

#endif // RELAY_HPP