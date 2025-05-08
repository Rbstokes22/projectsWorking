#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Light.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/MasterHandler.hpp"

namespace Comms {

#define SKT_BUF_SIZE 2048 // Large to accomodate the get all call
#define SKT_MAX_RESP_ARGS 5 // Max response arguments
#define SKT_REPLY_SIZE 128 // Basic replies
#define SKT_RANGE_EXC -999 // Default range value for exceptions.

// All commands sent by the client. Starts at index 1. When client passes
// numerical command, it corresponds to this enum, and will execute 
// appropriately.
enum class CMDS : uint8_t {
    GET_ALL = 1, CALIBRATE_TIME, NEW_LOG_RCVD, 
    RELAY_CTRL, RELAY_TIMER, ATTACH_RELAYS, 
    SET_TEMPHUM, SET_SOIL, SET_LIGHT, SET_SPEC_INTEGRATION_TIME, SET_SPEC_GAIN, 
    CLEAR_AVERAGES, CLEAR_AVG_SET_TIME, SAVE_AND_RESTART, GET_TRENDS
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

class SOCKHAND : public MASTERHAND {
    private:
    static char reportBuf[SKT_BUF_SIZE]; // Used for report sending in Alert.
    static const char* tag;
    static Peripheral::Relay* Relays; // Pointer to relays, init from main.cpp
    static bool isInit; // Shows if the handler is initialized.
    static argPool pool; // argument pool object.
    static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t* req, 
        async_resp_arg* arg);

    static void ws_async_send(void* arg);
    static void compileData(cmdData &data, char* buffer, size_t size);
    static bool inRange(int lower, int upper, int value, 
        int exception = SKT_RANGE_EXC, int multiplier = 1);
    static bool checkSTA(int &written, char* buffer, size_t size, 
        const char* reply, const char* idNum);
    
    static bool initCheck(httpd_req_t* req);
    static void THtrend(float* arr, char* buf, size_t bufSize, int iter); 
    static void lightTrend(uint16_t* arr, char* buf, size_t bufSize, 
        int iter);
    
    public:
    static bool init(Peripheral::Relay* relays);
    static esp_err_t wsHandler(httpd_req_t* req); // Entrance point.
    static void attachRelayTH(uint8_t relayNum, 
        Peripheral::TH_TRIP_CONFIG* conf, const char* caller);

    static void attachRelayLT(uint8_t relayNum,
        Peripheral::RelayConfigLight* conf, const char* caller);

    static char* getReportBuf();
};

}

#endif // SOCKETHANDLER_HPP