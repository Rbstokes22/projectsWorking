#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"

namespace Comms {

#define BUF_SIZE 256
#define MAX_RESP_ARGS 10

// extern Threads::Mutex* DHTmutex;
// extern Threads::Mutex* AS7341mutex;
// extern Threads::Mutex* SOILmutex;
// extern Peripheral::Relay* Relays;

enum class CMDS {
    GET_ALL = 1, 
    RELAY_1, RELAY_2, RELAY_3, RELAY_4,
    ATTACH_TEMP_RELAY, SET_TEMP_LWR_THAN, SET_TEMP_GTR_THAN,
    ATTACH_HUM_RELAY, SET_HUM_LWR_THAN, SET_HUM_GTR_THAN
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
    static Threads::Mutex* DHTmtx;
    static Threads::Mutex* AS7341mtx;
    static Threads::Mutex* SOILmtx;
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
    
    public:
    static bool init(
        Threads::Mutex &dhtMUTEX, 
        Threads::Mutex &as7341MUTEX, 
        Threads::Mutex &soilMUTEX,
        Peripheral::Relay* relays
        );
    static esp_err_t wsHandler(httpd_req_t* req);
    
};

}

#endif // SOCKETHANDLER_HPP