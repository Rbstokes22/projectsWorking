#ifndef NETWAP_HPP
#define NETWAP_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstdint>

namespace Comms {

struct WAPdetails {
    char ipaddr[static_cast<int>(IDXSIZE::IPADDR)];
    char status[4];
    char WAPtype[20];
    char heap[10];
    char clientConnected[4];
};

class NetWAP : public NetMain{
    private:
    char APssid[static_cast<int>(IDXSIZE::SSID)];
    char APpass[static_cast<int>(IDXSIZE::PASS)];
    bool isInit;
    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif;
    wifi_config_t wifi_config;
    httpd_config_t server_config;
    wifi_ret_t configure();
    wifi_ret_t dhcpsHandler();

    public:
    NetWAP(
        Messaging::MsgLogHandler &msglogerr, 
        const char* APssid, const char* APdefPass);
    wifi_ret_t init_wifi() override;
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    void setPhone(const char* phone) override;
    bool isInitialized() override;
    void setInit(bool newInit) override;
    void getDetails(WAPdetails &details);
    bool isBroadcasting();
    void setWAPtype(char* WAPtype);

};
    
}

#endif // NETWAP_HPP