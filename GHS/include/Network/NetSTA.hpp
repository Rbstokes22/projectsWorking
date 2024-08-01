#ifndef NETSTA_HPP
#define NETSTA_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstdint>

namespace Comms {

struct STAdetails {
char ssid[static_cast<int>(IDXSIZE::SSID)];
char ipaddr[static_cast<int>(IDXSIZE::IPADDR)];
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

    public:
    NetSTA(Messaging::MsgLogHandler &msglogerr);
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    void setPhone(const char* phone) override;
    bool isActive() override;
    void getDetails(STAdetails &details);
};

}

#endif // NETSTA_HPP