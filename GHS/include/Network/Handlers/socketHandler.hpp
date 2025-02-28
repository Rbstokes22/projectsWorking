#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Light.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

#define SKT_BUF_SIZE 2000 // Large to accomodate the get all call
#define SKT_MAX_RESP_ARGS 5 // Max response arguments

// All commands sent by the client. Starts at index 1. When client passes
// numerical command, it corresponds to this enum, and will execute 
// appropriately.
enum class CMDS : uint8_t {
    GET_ALL = 1, CALIBRATE_TIME, NEW_LOG_RCVD,
    RELAY_1, RELAY_2, RELAY_3, RELAY_4,
    RELAY_1_TIMER_ON, RELAY_1_TIMER_OFF, 
    RELAY_2_TIMER_ON, RELAY_2_TIMER_OFF,
    RELAY_3_TIMER_ON, RELAY_3_TIMER_OFF,
    RELAY_4_TIMER_ON, RELAY_4_TIMER_OFF,
    ATTACH_TEMP_RELAY, SET_TEMP_RE_LWR_THAN, SET_TEMP_RE_GTR_THAN,
    SET_TEMP_RE_COND_NONE, SET_TEMP_ALT_LWR_THAN, SET_TEMP_ALT_GTR_THAN,
    SET_TEMP_ALT_COND_NONE,
    ATTACH_HUM_RELAY, SET_HUM_RE_LWR_THAN, SET_HUM_RE_GTR_THAN, 
    SET_HUM_RE_COND_NONE, SET_HUM_ALT_LWR_THAN, SET_HUM_ALT_GTR_THAN,
    SET_HUM_ALT_COND_NONE, CLEAR_TEMPHUM_AVG,
    SET_SOIL1_LWR_THAN, SET_SOIL1_GTR_THAN, SET_SOIL1_COND_NONE,
    SET_SOIL2_LWR_THAN, SET_SOIL2_GTR_THAN, SET_SOIL2_COND_NONE,
    SET_SOIL3_LWR_THAN, SET_SOIL3_GTR_THAN, SET_SOIL3_COND_NONE,
    SET_SOIL4_LWR_THAN, SET_SOIL4_GTR_THAN, SET_SOIL4_COND_NONE,
    ATTACH_LIGHT_RELAY, SET_LIGHT_RE_LWR_THAN, SET_LIGHT_RE_GTR_THAN,
    SET_LIGHT_RE_COND_NONE, SET_DARK_VALUE, CLEAR_LIGHT_AVG,
    SEND_REPORT_SET_TIME, SAVE_AND_RESTART
};

struct cmdData { // Command Data
    CMDS cmd; // uint8_t command.
    int suppData; // Supplementary data included with command.
    char idNum[20]; // Request ID number sent by client.
};

struct async_resp_arg { // Socket response argument struct.
    httpd_handle_t hd; // handle
    int fd; // file descriptior
    uint8_t Buf[SKT_BUF_SIZE]; // Data buffer.
    cmdData data; // Command data.
};

class argPool { 

    // Want static allocation of the argument pool to avoid using the HEAP
    // which can lead to memory fragmentation.
    private:
    async_resp_arg pool[SKT_MAX_RESP_ARGS]; // Pool of arguemnts
    bool inUse[SKT_MAX_RESP_ARGS]; // flag to show which pool is in use.

    public:
    argPool();
    async_resp_arg* getArg();
    void releaseArg(async_resp_arg* arg);
};

// NOTE: All static variables and functions due to requirements of the URI
// handlers. Could be a singleton, but didnt want to rewrite. 

class SOCKHAND {
    private:
    static const char* tag;
    static char log[LOG_MAX_ENTRY]; // error log.
    static Peripheral::Relay* Relays; // Pointer to relays, init from main.cpp
    static bool isInit; // Shows if the handler is initialized.
    static argPool pool; // argument pool object.
    static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t* req, 
        async_resp_arg* arg);

    static void ws_async_send(void* arg);
    static void compileData(cmdData &data, char* buffer, size_t size);
    static bool inRange(int lower, int upper, int value, int exception = -999);
    static bool checkSTA(int &written, char* buffer, size_t size, 
        const char* reply, const char* idNum);
    
    static void sendErr(const char* msg, 
        Messaging::Levels lvl = Messaging::Levels::ERROR);
    
    public:
    static bool init(Peripheral::Relay* relays);
    static esp_err_t wsHandler(httpd_req_t* req);
    static void attachRelayTH(uint8_t relayNum, 
        Peripheral::TH_TRIP_CONFIG* conf);

    static void attachRelayLT(uint8_t relayNum,
        Peripheral::RelayConfigLight* conf);
};

}

#endif // SOCKETHANDLER_HPP