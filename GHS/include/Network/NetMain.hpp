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

namespace Comms {

enum class wifi_ret_t {
    INIT_OK, INIT_FAIL, WIFI_OK, WIFI_FAIL,
    SERVER_OK, SERVER_FAIL, DESTROY_OK, DESTROY_FAIL
};

class NetMain {
    protected:
    httpd_handle_t server;
    Messaging::MsgLogHandler &msglogerr;
    static NetMode NetType;
    static char ssid[static_cast<int>(IDXSIZE::SSID)];
    static char pass[static_cast<int>(IDXSIZE::PASS)];
    static char phone[static_cast<int>(IDXSIZE::PHONE)];
    
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