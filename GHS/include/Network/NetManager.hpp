#ifndef NETMANAGER_HPP
#define NETMANAGER_HPP

#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "UI/Display.hpp"

namespace Comms {

class NetManager {
    private:
    NetSTA &station;
    NetWAP &wap;
    UI::Display &OLED;
    bool isWifiInit;
    NetMode checkNetSwitch();
    void setNetType(NetMode netType);
    void checkConnection(NetMain &mode, NetMode NetType);
    void startServer(NetMain &mode);
    void runningWifi(NetMain &mode);
    void reconnect(NetMain &mode, uint8_t &attempt);

    public:
    NetManager(
        NetSTA &station, 
        NetWAP &wap,  
        UI::Display &OLED
        );

    void handleNet();
};

}



#endif // NETMANAGER_HPP