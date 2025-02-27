#ifndef NETWAP_HPP
#define NETWAP_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstdint>

namespace Comms {

struct WAPdetails { // Wireless Access Point details used for OLED print.
    // +9 for http:// 
    char ipaddr[static_cast<int>(IDXSIZE::IPADDR) + 9]; 

    // +9 for http:// +6 for .local
    char mdns[static_cast<int>(IDXSIZE::MDNS) + 15]; 
    char status[4]; // yes or no if active.
    char WAPtype[20]; // shows WAP or WAP SETUP.
    char heap[20]; // heap allocation remaining
    char clientConnected[4]; // total clients connected.
};

class NetWAP : public NetMain{
    private:
    const char* tag;
    char APssid[static_cast<int>(IDXSIZE::SSID)]; // WAP ssid/user
    char APpass[static_cast<int>(IDXSIZE::PASS)]; // WAP password
    char APdefaultPass[static_cast<int>(IDXSIZE::PASS)]; // Default password
    esp_netif_ip_info_t ip_info; // Network interface IP information.
    wifi_ret_t configure() override;
    wifi_ret_t dhcpsHandler();

    public:
    NetWAP(const char* APssid, const char* APdefPass, const char* mdnsName);
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