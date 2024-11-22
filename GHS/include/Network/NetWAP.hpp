#ifndef NETWAP_HPP
#define NETWAP_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstdint>

namespace Comms {

struct WAPdetails {
    // +9 for http:// 
    char ipaddr[static_cast<int>(IDXSIZE::IPADDR) + 9]; 

    // +9 for http:// +6 for .local
    char mdns[static_cast<int>(IDXSIZE::MDNS) + 15]; 
    char status[4];
    char WAPtype[20];
    char heap[20];
    char clientConnected[4];
};

class NetWAP : public NetMain{
    private:
    char APssid[static_cast<int>(IDXSIZE::SSID)];
    char APpass[static_cast<int>(IDXSIZE::PASS)];
    char APdefaultPass[static_cast<int>(IDXSIZE::PASS)];
    esp_netif_ip_info_t ip_info;
    wifi_ret_t configure() override;
    wifi_ret_t dhcpsHandler();

    public:
    NetWAP(
        Messaging::MsgLogHandler &msglogerr, 
        const char* APssid, const char* APdefPass,
        const char* mdnsName);
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    const char* getPass(bool def = false) const override;
    const char* getSSID() const override;
    bool isActive() override;
    void getDetails(WAPdetails &details);
    void setWAPtype(char* WAPtype);

};
    
}

#endif // NETWAP_HPP