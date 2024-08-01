#include "Network/NetManager.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "driver/gpio.h"
#include "config.hpp"
#include "UI/Display.hpp"

namespace Comms {

NetManager::NetManager(NetSTA &station, NetWAP &wap, NVS::Creds &creds, UI::Display &OLED) :

    station{station}, wap{wap}, creds(creds), OLED(OLED), isWifiInit(false) {}

void NetManager::netStart(NetMode NetType) {

    if (NetType == NetMode::WAP || NetType == NetMode::WAP_SETUP) {
        this->checkConnection(this->wap, NetType);
    } else if (NetType == NetMode::STA) {
        this->checkConnection(this->station, NetType);
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
void NetManager::startServer(NetMain &mode) { // HANDLE DEFAULT BUTTON FOR AP
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

    // This block ensures that each previous block is properly initialize
    // before it continues to the next function. All error handling is 
    // embedded within these functions.
    if (mode.init_wifi() == wifi_ret_t::INIT_OK) {
        if (mode.start_wifi() == wifi_ret_t::WIFI_OK) {
            mode.start_server();
        }
    }
}

void NetManager::checkConnection(NetMain &mode, NetMode NetType) {
    NetMode net_type = mode.getNetType();

    if (NetType != net_type) { // Checks if the mode has been switched

        if (net_type == NetMode::WAP || net_type == NetMode::WAP_SETUP) {
            this->wap.destroy();
        } else if (net_type == NetMode::STA) {
            this->station.destroy();
        }
    
        mode.setNetType(NetType);
        this->startServer(mode);
    }

    else {

        this->runningWifi(mode);
    }
}

void NetManager::runningWifi(NetMain &mode) {
    NetMode curSrvr = mode.getNetType();
    static uint8_t reconAttempt{0};

    if (curSrvr == NetMode::WAP || curSrvr == NetMode::WAP_SETUP) {
        WAPdetails details;
        wap.getDetails(details);
        OLED.printWAP(details);

    } else if (curSrvr == NetMode::STA) {
        STAdetails details;
        station.getDetails(details);
        OLED.printSTA(details);
    }   

    // If not connected, will attempt to continue starting the server.
    if (!mode.isActive()) {
        reconnect(mode, reconAttempt);
    } else {
        reconAttempt = 0;
    }
}

// If connection is inactive, will attempt to start the server cleaning
// up any pieces that havent been initialized. If all fails after 10 
// attempts, the current mode will be destroyed allowing fresh init.
void NetManager::reconnect(NetMain &mode, uint8_t &attempt) {

    startServer(mode);
    attempt++;

    if (attempt > 10) {
        mode.destroy();
        attempt = 0;
    }
}

}