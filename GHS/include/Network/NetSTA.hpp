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
char heap[10];
};

class NetSTA : public NetMain {
    private:
    char ssid[static_cast<int>(IDXSIZE::SSID)];
    char pass[static_cast<int>(IDXSIZE::PASS)];
    char phone[static_cast<int>(IDXSIZE::PHONE)];
    static char IPADDR[static_cast<int>(IDXSIZE::IPADDR)];
    bool isInit;
    wifi_ret_t configure(wifi_config_t &wifi_config);
    static void IPEvent( // used to get IP addr
        void* arg, 
        esp_event_base_t event, 
        int32_t eventID, 
        void* eventData);

    public:
    NetSTA(Messaging::MsgLogHandler &msglogerr);
    wifi_ret_t init_wifi() override;
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    void setPhone(const char* phone) override;
    bool isInitialized() override;
    void setInit(bool newInit) override;
    void getDetails(STAdetails &details);
    bool isConnected();

};

}

#endif // NETSTA_HPP