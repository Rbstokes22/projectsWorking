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
        const char* APssid, const char* APdefPass);
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    void setPhone(const char* phone) override;
    const char* getPass(bool def = false) const override;
    const char* getSSID() const override;
    const char* getPhone() const override;
    bool isActive() override;
    void getDetails(WAPdetails &details);
    void setWAPtype(char* WAPtype);

};
    
}

#endif // NETWAP_HPP