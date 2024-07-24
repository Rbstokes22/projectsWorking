#ifndef NETMANAGER_HPP
#define NETMANAGER_HPP

#include "driver/gpio.h"
#include "config.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Network/NetConfig.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"

namespace Communications {

class NetManager {
    private:
    NetSTA &station;
    NetWAP &wap;

    public:
    NetManager(
        Messaging::MsgLogHandler &msglogerr,
        NetSTA &station, NetWAP &wap
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