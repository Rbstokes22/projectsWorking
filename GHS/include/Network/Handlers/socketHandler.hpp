#ifndef SOCKETHANDLER_HPP
#define SOCKETHANDLER_HPP

#include "esp_http_server.h"
#include "Network/NetSTA.hpp"

namespace Comms {

#define BUF_SIZE 512
#define MAX_RESP_ARGS 10

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

extern struct async_resp_arg argPool[MAX_RESP_ARGS];
extern bool inUse[MAX_RESP_ARGS];

void setNetObjs(NetSTA &_station);
struct async_resp_arg* getArg();
void releaseArg(struct async_resp_arg* arg);
esp_err_t wsHandler(httpd_req_t* req);
esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t* req);
void ws_async_send(void* arg);


}

#endif // SOCKETHANDLER_HPP