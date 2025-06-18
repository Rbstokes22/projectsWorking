#ifndef NETSTA_HPP
#define NETSTA_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstdint>

namespace Comms {

struct STAdetails { // Station details used for printing to OLED
char ssid[static_cast<int>(IDXSIZE::SSID)];

// +9 for http:// 
char ipaddr[static_cast<int>(IDXSIZE::IPADDR) + 9]; 

// +9 for http:// +6 for .local
char mdns[static_cast<int>(IDXSIZE::MDNS) + 15]; 
char signalStrength[12]; // rssi returned by device.
char status[4]; // yes or no for connected
char heap[20]; // heap allocation remaining.
};

class NetSTA : public NetMain {
    private:
    const char* tag;
    char ssid[static_cast<int>(IDXSIZE::SSID)]; // station network ssid/user
    char pass[static_cast<int>(IDXSIZE::PASS)]; // station network password
    static char IPADDR[static_cast<int>(IDXSIZE::IPADDR)]; // station net IP
    wifi_ret_t configure() override;
    static void IPEvent(void* arg, esp_event_base_t event, int32_t eventID, 
        void* eventData); // Used to get IP address.

    public:
    NetSTA(const char* mdnsName);
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    const char* getPass(bool def = false) const override;
    const char* getSSID() const override;
    bool isActive() override;
    void getDetails(STAdetails &details);
    wifi_sta_config_t* getConf();
};

}

#endif // NETSTA_HPP