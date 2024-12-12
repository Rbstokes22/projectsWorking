#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"

namespace Comms {

#define BUF_SIZE 1024 // Large to accomodate the get all call
#define MAX_RESP_ARGS 10

enum class CMDS : uint8_t {
    GET_ALL = 1, CALIBRATE_TIME,
    RELAY_1, RELAY_2, RELAY_3, RELAY_4,
    RELAY_1_TIMER_ON, RELAY_1_TIMER_OFF, 
    RELAY_2_TIMER_ON, RELAY_2_TIMER_OFF,
    RELAY_3_TIMER_ON, RELAY_3_TIMER_OFF,
    RELAY_4_TIMER_ON, RELAY_4_TIMER_OFF,
    ATTACH_TEMP_RELAY, SET_TEMP_LWR_THAN, SET_TEMP_GTR_THAN,
    // Work here the same stuff for the relay as the alert,
    // de couple it, so that a user may set alert conditions
    // independently from relay.

    SET_TEMP_COND_NONE, ENABLE_TEMP_ALERT, DISABLE_TEMP_ALERT,
    ATTACH_HUM_RELAY, SET_HUM_LWR_THAN, SET_HUM_GTR_THAN, 
    SET_HUM_COND_NONE, ENABLE_HUM_ALERT, DISABLE_HUM_ALERT,
    SET_SOIL1_LWR_THAN, SET_SOIL1_GTR_THAN, SET_SOIL1_COND_NONE,
    SET_SOIL2_LWR_THAN, SET_SOIL2_GTR_THAN, SET_SOIL2_COND_NONE,
    SET_SOIL3_LWR_THAN, SET_SOIL3_GTR_THAN, SET_SOIL3_COND_NONE,
    SET_SOIL4_LWR_THAN, SET_SOIL4_GTR_THAN, SET_SOIL4_COND_NONE, 
    TEST1, TEST2
};

// Commands sent by the client to include command, supplementary
// data, and the idNum to send it back to.
struct cmdData {
    CMDS cmd;
    int suppData;
    char idNum[20];
};

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    uint8_t Buf[BUF_SIZE];
    cmdData data;
};

class argPool {
    private:
    async_resp_arg pool[MAX_RESP_ARGS];
    bool inUse[MAX_RESP_ARGS];

    public:
    argPool();
    async_resp_arg* getArg();
    void releaseArg(async_resp_arg* arg);
};

class SOCKHAND {
    private:
    static Peripheral::Relay* Relays;
    static bool isInit;
    static argPool pool;
    static esp_err_t trigger_async_send(
        httpd_handle_t handle, 
        httpd_req_t* req, 
        async_resp_arg* arg
        );
    static void ws_async_send(void* arg);
    static void compileData(cmdData &data, char* buffer, size_t size);
    static void attachRelayTH(
        uint8_t relayNum, 
        Peripheral::TH_TRIP_CONFIG* conf
        );
    static int inRange(int lower, int upper, int value);
    
    public:
    static bool init(Peripheral::Relay* relays);
    static esp_err_t wsHandler(httpd_req_t* req);
    
};

}

#endif // SOCKETHANDLER_HPP