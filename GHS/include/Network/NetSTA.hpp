#ifndef NETSTA_HPP
#define NETSTA_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"

namespace Comms {

class NetSTA : public NetMain {
    private:
    char ssid[static_cast<int>(IDXSIZE::SSID)];
    char pass[static_cast<int>(IDXSIZE::PASS)];
    char phone[static_cast<int>(IDXSIZE::PHONE)];
    wifi_ret_t configure(wifi_config_t &wifi_config);

    public:
    NetSTA(Messaging::MsgLogHandler &msglogerr);
    wifi_ret_t init_wifi() override;
    wifi_ret_t start_wifi() override;
    wifi_ret_t start_server() override;
    wifi_ret_t destroy() override;
    void setPass(const char* pass) override;
    void setSSID(const char* ssid) override;
    void setPhone(const char* phone) override;

};

}

#endif // NETSTA_HPP