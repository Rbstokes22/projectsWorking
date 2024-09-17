#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"

namespace Comms {

#define BUF_SIZE 256
#define MAX_RESP_ARGS 10

enum class CMDS {
    GET_ALL = 1, SET_MIN_TEMP, SET_MAX_TEMP, SET_MIN_HUM,
    SET_MAX_HUM
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

extern struct async_resp_arg argPool[MAX_RESP_ARGS];
extern bool inUse[MAX_RESP_ARGS];

struct async_resp_arg* getArg();
void releaseArg(struct async_resp_arg* arg);
esp_err_t wsHandler(httpd_req_t* req);
esp_err_t trigger_async_send(
    httpd_handle_t handle, 
    httpd_req_t* req, 
    async_resp_arg* arg
    );

void ws_async_send(void* arg);
void compileData(cmdData &data, char* buffer, size_t size);

}

#endif // SOCKETHANDLER_HPP