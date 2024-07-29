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
    NVS::Creds &creds;
    UI::Display &OLED;

    public:
    NetManager(NetSTA &station, NetWAP &wap, NVS::Creds &creds, UI::Display &OLED);
    void netStart(NetMode netType);
    void handleNet();
    NetMode checkNetSwitch();
    void startServer(NetMain &mode);
    void checkConnection(NetMain &mode, NetMode NetType);
    void runningWifi(NetMain &mode);
};

}



#endif // NETMANAGER_HPP