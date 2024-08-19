#ifndef NETSTA_HPP
#define NETSTA_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include "NetCreds.hpp"
#include <cstdint>

namespace Comms {

struct STAdetails {
char ssid[static_cast<int>(IDXSIZE::SSID)];

// +9 for http:// 
char ipaddr[static_cast<int>(IDXSIZE::IPADDR) + 9]; 

// +9 for http:// +6 for .local
char mdns[static_cast<int>(IDXSIZE::MDNS) + 15]; 
char signalStrength[12];
char status[4];
char heap[20];
};

class NetSTA : public NetMain {
    private:
    char ssid[static_cast<int>(IDXSIZE::SSID)];
    char pass[static_cast<int>(IDXSIZE::PASS)];
    char phone[static_cast<int>(IDXSIZE::PHONE)];
    static char IPADDR[static_cast<int>(IDXSIZE::IPADDR)];
    wifi_ret_t configure() override;
    static void IPEvent( // used to get IP addr
        void* arg, 
        esp_event_base_t event, 
        int32_t eventID, 
        void* eventData);
    NVS::Creds &creds;

    public:
    NetSTA(
        Messaging::MsgLogHandler &msglogerr, 
        NVS::Creds &creds, const char* mdnsName);
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
    void getDetails(STAdetails &details);
};

}

#endif // NETSTA_HPP