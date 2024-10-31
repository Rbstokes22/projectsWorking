#ifndef NETMAIN_HPP
#define NETMAIN_HPP

#include "Config/config.hpp"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

enum class wifi_ret_t { // wifi return type
    INIT_OK, INIT_FAIL, WIFI_OK, WIFI_FAIL,
    SERVER_OK, SERVER_FAIL, DESTROY_OK, DESTROY_FAIL,
    CONFIG_OK, CONFIG_FAIL, DHCPS_OK, DHCPS_FAIL,
    MDNS_OK, MDNS_FAIL
};

enum class HEAP_SIZE {
    B, KB, MB
};

// Error display method.
enum class errDisp {
    OLED, SRL, ALL
};

// Flags to ensure a linear flow in the net init process.
struct Flags { 
    bool wifiInit;
    bool netifInit;
    bool eventLoopInit;
    bool ap_netifCreated;
    bool sta_netifCreated;
    bool dhcpOn; // must set to true to explicity call stop
    bool dhcpIPset;
    bool wifiModeSet;
    bool wifiConfigSet;
    bool wifiOn;
    bool httpdOn;
    bool uriReg;
    bool handlerReg;
    bool wifiConnected;
    bool mdnsInit;
    bool mdnsHostSet;
    bool mdnsInstanceSet;
    bool mdnsServiceAdded;
};

class NetMain {
    private:

    protected:
    static httpd_handle_t server;
    Messaging::MsgLogHandler &msglogerr;
    static NetMode NetType;
    static Flags flags; // to check for initialization
    static esp_netif_t* ap_netif;
    static esp_netif_t* sta_netif;
    static wifi_init_config_t init_config;
    wifi_config_t wifi_config;
    static httpd_config http_config;
    char mdnsName[static_cast<int>(IDXSIZE::MDNS)];

    public:
    NetMain(Messaging::MsgLogHandler &msglogerr, const char* mdnsName);
    virtual ~NetMain();
    wifi_ret_t init_wifi();
    wifi_ret_t mDNS();
    virtual wifi_ret_t configure() = 0;
    virtual wifi_ret_t start_wifi() = 0;
    virtual wifi_ret_t start_server() = 0; 
    virtual wifi_ret_t destroy() = 0;
    virtual void setPass(const char* pass) = 0;
    virtual void setSSID(const char* ssid) = 0;
    virtual void setPhone(const char* phone) = 0;
    virtual const char* getPass(bool def = false) const = 0;
    virtual const char* getSSID() const = 0;
    virtual const char* getPhone() const = 0;
    virtual bool isActive() = 0;
    NetMode getNetType();
    void setNetType(NetMode NetType);
    float getHeapSize(HEAP_SIZE type);
    httpd_handle_t getServer();
    void sendErr(const char* msg, errDisp type);
};

}

#endif // NETMAIN_HPP