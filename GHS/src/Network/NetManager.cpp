#include "Network/NetManager.hpp"

namespace Communications {

// USE THE NVS HERE FOR ALL SAVED SETTINGS FOR CREDENTIALS AND PASS TO CLASS METHODS

NetManager::NetManager(
    Messaging::MsgLogHandler &msglogerr,
    NetSTA &station, NetWAP &wap) :

    station{station}, wap{wap} {}

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

void NetManager::restartServer(NetMain &mode) {
    mode.init_wifi();
    mode.start_wifi();
    mode.start_server();
}

void NetManager::checkConnection(NetMain &mode, NetMode NetType) {
    NetMode net_type = mode.getNetType();

    if (NetType != net_type) {
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

        restartServer(mode);
        mode.setNetType(NetType);
    }

    else {
        runningWifi(mode);
    }
}

void NetManager::runningWifi(NetMain &mode) {
    // ping the running server here.mode.run or whatever.
    // or make it check connections, rssi, etc...
}

}