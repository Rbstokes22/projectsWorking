#ifndef NETMANAGER_HPP
#define NETMANAGER_HPP

#include "UI/MsgLogHandler.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"

namespace Comms {

class NetManager {
    private:
    NetSTA &station;
    NetWAP &wap;
    NVS::Creds &creds;

    public:
    NetManager(
        Messaging::MsgLogHandler &msglogerr,
        NetSTA &station, NetWAP &wap,
        NVS::Creds &creds
    );
    void netStart(NetMode netType);
    void handleNet();
    NetMode checkNetSwitch();
    void restartServer(NetMain &mode);
    void checkConnection(NetMain &mode, NetMode NetType);
    void runningWifi(NetMain &mode);
};

}



#endif // NETMANAGER_HPP