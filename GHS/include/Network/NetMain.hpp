#ifndef NETMAIN_HPP
#define NETMAIN_HPP

#include "config.hpp"
#include "NetConfig.hpp"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "UI/MsgLogHandler.hpp"

namespace Communications {

class NetMain {
    protected:
    httpd_handle_t server;
    Messaging::MsgLogHandler &msglogerr;
    static NetMode NetType;
    
    public:
    NetMain(Messaging::MsgLogHandler &msglogerr);
    virtual ~NetMain();
    virtual void init_wifi() = 0;
    virtual void start_wifi() = 0;
    virtual void start_server() = 0;
    virtual void destroy() = 0;
    virtual NetMode getNetType();
    virtual void setNetType(NetMode NetType);

};

}

#endif // NETMAIN_HPP