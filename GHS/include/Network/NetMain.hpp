#ifndef NETMAIN_HPP
#define NETMAIN_HPP

#include "Config/config.hpp"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "Common/FlagReg.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

enum class wifi_ret_t { // wifi return type
    INIT_OK, INIT_FAIL, WIFI_OK, WIFI_FAIL,
    SERVER_OK, SERVER_FAIL, DESTROY_OK, DESTROY_FAIL,
    CONFIG_OK, CONFIG_FAIL, DHCPS_OK, DHCPS_FAIL,
    MDNS_OK, MDNS_FAIL
};

enum class HEAP_SIZE { // Heap units used in display.
    B, KB, MB
};

// Netflags that hold bit/idx positions for on/off flags in the Flag class.
enum NETFLAGS : uint8_t { 
    wifiInit, netifInit, eventLoopInit, ap_netifCreated, sta_netifCreated,
    dhcpOn, dhcpIPset, wifiModeSet, wifiConfigSet, wifiOn, httpdOn, uriReg,
    handlerReg, wifiConnected, mdnsInit, mdnsHostSet, mdnsInstanceSet,
    mdnsServiceAdded
};

class NetMain {
    private:
    const char* tag; // Used for logging

    protected:
    static httpd_handle_t server; // server handle.
    static NetMode NetType; // Network type.
    static Flag::FlagReg flags; // on/off flags.
    static esp_netif_t* ap_netif; // Wireless Access point network interface.
    static esp_netif_t* sta_netif; // Station network interface.
    static wifi_init_config_t init_config; // Initialization configuration.
    wifi_config_t wifi_config; // Wifi configuration.
    static httpd_config http_config; // httpd configuration.
    char mdnsName[static_cast<int>(IDXSIZE::MDNS)]; // multicast DNS name. 

    public:
    NetMain(const char* mdnsName);
    virtual ~NetMain();
    wifi_ret_t init_wifi();
    wifi_ret_t mDNS();
    virtual wifi_ret_t configure() = 0;
    virtual wifi_ret_t start_wifi() = 0;
    virtual wifi_ret_t start_server() = 0; 
    virtual wifi_ret_t destroy() = 0;
    virtual void setPass(const char* pass) = 0;
    virtual void setSSID(const char* ssid) = 0;
    virtual const char* getPass(bool def = false) const = 0;
    virtual const char* getSSID() const = 0;
    virtual bool isActive() = 0;
    static NetMode getNetType();
    void setNetType(NetMode NetType);
    float getHeapSize(HEAP_SIZE type);
    httpd_handle_t getServer();
    void sendErr(const char* msg, 
            Messaging::Levels lvl = Messaging::Levels::ERROR); 
};

}

#endif // NETMAIN_HPP