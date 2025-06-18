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
#define NET_MAX_AP_SCAN 20 // How many access points will be detectable.
#define NET_SCAN_MIN_WAIT 10 // Scans every n minutes
#define NET_STA_RSSI_MIN -50 // value in dBm to attempt reconnect.
#define NET_SCAN_HYSTERESIS 5 // Difference to signal reconnect to new AP.

enum class scan_ret_t : uint8_t {
    SCAN_OK_UPD, // Scan OK, found better connection, updated.
    SCAN_OK_NOT_UPD, // Scan OK, did not find better connection.
    SCAN_NOT_REQ, // RSSI within params.
    SCAN_ERR, // Attempted scan, had error.
    SCAN_AWAITING // Scan is not within time limits
};

class NetManager {
    private:
    const char* tag; 
    char log[LOG_MAX_ENTRY]; // internal error handling.
    NetSTA &station; // Reference to station object.
    NetWAP &wap; // Reference to wireless access point object.
    UI::Display &OLED; // Reference to OLED display for network details.
    NetMode checkNetSwitch();
    void setNetType(NetMode netType);
    void checkConnection(NetMain &mode, NetMode NetType);
    void startServer(NetMain &mode);
    void runningWifi(NetMain &mode);
    void reconnect(NetMain &mode, uint8_t &attempt);
    bool handleDestruction(NetMain &mode);
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::INFO, bool ignoreRepeat = false);
    
    public:
    NetManager(NetSTA &station, NetWAP &wap,  UI::Display &OLED);
    void handleNet();
    scan_ret_t scan();
};

}



#endif // NETMANAGER_HPP