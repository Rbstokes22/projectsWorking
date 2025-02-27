#ifndef NETMANAGER_HPP
#define NETMANAGER_HPP

#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "UI/Display.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

#define NET_ATTEMPTS_RECON 3 // reconnection attempts before net restart
#define NET_ATTEMPTS_DESTROY 3 // tries to destroy a connection
#define NET_DESTROY_FAIL_FORCE_RESET true // Forces reset if destruction fails.

class NetManager {
    private:
    const char* tag; 
    char log[LOG_MAX_ENTRY]; // internal error handling.
    NetSTA &station; // Reference to station object.
    NetWAP &wap; // Reference to wireless access point object.
    UI::Display &OLED; // Reference to OLED display for network details.
    bool isWifiInit; // Is wifi initialized. 
    NetMode checkNetSwitch();
    void setNetType(NetMode netType);
    void checkConnection(NetMain &mode, NetMode NetType);
    void startServer(NetMain &mode);
    void runningWifi(NetMain &mode);
    void reconnect(NetMain &mode, uint8_t &attempt);
    bool handleDestruction(NetMain &mode);

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