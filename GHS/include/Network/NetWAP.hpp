#ifndef NETWAP_HPP
#define NETWAP_HPP

#include "Network/NetMain.hpp"
#include "esp_wifi.h"


namespace Comms {

class NetWAP : public NetMain{
    private:
    char APssid[static_cast<int>(IDXSIZE::SSID)];
    char APpass[static_cast<int>(IDXSIZE::PASS)];
    wifi_ret_t configure(wifi_config_t &wifi_config, uint8_t maxConnections);

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
    

};
    
}

#endif // NETWAP_HPP