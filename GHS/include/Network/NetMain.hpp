#ifndef NETMAIN_HPP
#define NETMAIN_HPP

#include "NetConfig.hpp"
#include "esp_http_server.h"
#include "UI/MsgLogHandler.hpp"

// FIX ALL INCLUDES TO PUT INTO SOURCE FILES. WORK ON OLED NET DISPLAY AND HEAP
namespace Comms {

enum class wifi_ret_t { // wifi return type
    INIT_OK, INIT_FAIL, WIFI_OK, WIFI_FAIL,
    SERVER_OK, SERVER_FAIL, DESTROY_OK, DESTROY_FAIL,
    CONFIG_OK, CONFIG_FAIL
};

class NetMain {
    protected:
    httpd_handle_t server;
    Messaging::MsgLogHandler &msglogerr;
    static NetMode NetType;

    public:
    NetMain(Messaging::MsgLogHandler &msglogerr);
    virtual ~NetMain();
    virtual wifi_ret_t init_wifi() = 0;
    virtual wifi_ret_t start_wifi() = 0;
    virtual wifi_ret_t start_server() = 0;
    virtual wifi_ret_t destroy() = 0;
    virtual void setPass(const char* pass) = 0;
    virtual void setSSID(const char* ssid) = 0;
    virtual void setPhone(const char* phone) = 0;
    virtual NetMode getNetType();
    virtual void setNetType(NetMode NetType);
    void sendErr(const char* msg);

};

}

#endif // NETMAIN_HPP