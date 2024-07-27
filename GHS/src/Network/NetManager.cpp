#include "Network/NetManager.hpp"
#include "driver/gpio.h"
#include "config.hpp"

namespace Comms {

NetManager::NetManager(
    Messaging::MsgLogHandler &msglogerr,
    NetSTA &station, NetWAP &wap,
    NVS::Creds &creds) :

    station{station}, wap{wap}, creds(creds) {}

void NetManager::netStart(NetMode NetType) {

    switch(NetType) {
        case NetMode::WAP:
        this->checkConnection(this->wap, NetType);
        break;

        case NetMode::WAP_SETUP:
        this->checkConnection(this->wap, NetType);
        break;

        case NetMode::STA:
        this->checkConnection(this->station, NetType);
        break;

        case NetMode::NONE:
        break;
    } 
}

void NetManager::handleNet() {
    NetMode mode = this->checkNetSwitch();
    netStart(mode);
}

NetMode NetManager::checkNetSwitch() {
    bool WAP = gpio_get_level(pinMapD[static_cast<uint8_t>(DPIN::WAP)]);
    bool STA = gpio_get_level(pinMapD[static_cast<uint8_t>(DPIN::STA)]);

    // activation means dropped down to ground.
    if (WAP && !STA) {return NetMode::STA;}
    else if (!WAP && STA) {return NetMode::WAP;}
    else if (WAP && STA) {return NetMode::WAP_SETUP;}
    else {return NetMode::NONE;}
}

// Upon the start of any connection, the NVS is checked for creds. If it returns
// nothing, then the default settings will be used. If data does exist, those 
// values will be used.
void NetManager::restartServer(NetMain &mode) { // HANDLE DEFAULT BUTTON FOR AP
    NetMode curSrvr = mode.getNetType();

    // All logic depending on returns from the read functions, will be handled
    // in the WAP and STA classes.
    if (curSrvr == NetMode::WAP || curSrvr == NetMode::WAP_SETUP) {

        mode.setPass(this->creds.read("APpass")); 
        
    } else if (curSrvr == NetMode::STA) {
        
        mode.setPass(this->creds.read("pass"));
        mode.setSSID(this->creds.read("ssid"));
        mode.setPhone(this->creds.read("phone"));

    }

    mode.init_wifi(); 
    mode.start_wifi();
    mode.start_server();
}

void NetManager::checkConnection(NetMain &mode, NetMode NetType) {
    NetMode net_type = mode.getNetType();

    if (NetType != net_type) { // Checks if the mode has been switched
        switch (net_type) {
            case NetMode::WAP:
            this->wap.destroy();
            break;

            case NetMode::WAP_SETUP:
            this->wap.destroy();
            break;

            case NetMode::STA:
            this->station.destroy();
            break;

            case NetMode::NONE:
            break;
        }

        mode.setNetType(NetType);
        this->restartServer(mode);
        
    }

    else {
        this->runningWifi(mode);
    }
}

void NetManager::runningWifi(NetMain &mode) {
    // ping the running server here.mode.run or whatever.
    // or make it check connections, rssi, etc...
}

}