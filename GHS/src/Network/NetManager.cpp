#include "Network/NetManager.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "driver/gpio.h"
#include "config.hpp"
#include "UI/Display.hpp"

namespace Comms {

// Checks the reading of the GPIO pins to determine if the switch is 
// in station, station setup, or wap mode. Returns NetMode STA, WAP,
// and WAP_SETUP.
NetMode NetManager::checkNetSwitch() {
    bool WAP = gpio_get_level(pinMapD[static_cast<uint8_t>(DPIN::WAP)]);
    bool STA = gpio_get_level(pinMapD[static_cast<uint8_t>(DPIN::STA)]);

    // activation means dropped down to ground.
    if (WAP && !STA) {return NetMode::STA;}
    else if (!WAP && STA) {return NetMode::WAP;}
    else if (WAP && STA) {return NetMode::WAP_SETUP;}
    else {return NetMode::NONE;}
}

// Determines if the set position is that of a WAP or STA. Checks connection
// based on this type.
void NetManager::setNetType(NetMode NetType) {

    if (NetType == NetMode::WAP || NetType == NetMode::WAP_SETUP) {
        this->checkConnection(this->wap, NetType);
    } else if (NetType == NetMode::STA) {
        this->checkConnection(this->station, NetType);
    }
}

// Gets the current net type registered. If that net type is not equal to
// the switch position, it will destroy the current wifi connection,
// set the new net type, and call to start the server. If the connection
// is already present, executes the running wifi method instead. 
void NetManager::checkConnection(NetMain &mode, NetMode NetType) {
    NetMode net_type = mode.getNetType();

    if (NetType != net_type) { // Checks if the mode has been switched

        // WAP_RECON will be flagged upon a change of the 
        // WAP pass. This will allow a new connection using new creds.
        if (net_type == NetMode::WAP || net_type == NetMode::WAP_SETUP || 
            net_type == NetMode::WAP_RECON) {
            this->wap.destroy();
        } else if (net_type == NetMode::STA) {
            this->station.destroy();
        } 
    
        // startServer() will be called exactly one time before the 
        // program goes into runningWifi() which assumes a solid
        // init. In the event this fails, startServer() is also called 
        // when attempting a reconnection in other methods.
        mode.setNetType(NetType);
        this->startServer(mode);
    }

    else {

        this->runningWifi(mode);
    }
}

// NVS is checked for credentials upon wifi init. Default values will be used
// if data is invalid, which means the AP will start but the ST will not. If the
// default button is held upon the startup, it will start with the default 
// password.
void NetManager::startServer(NetMain &mode) { 
    NetMode curSrvr = mode.getNetType();

    // All logic depending on returns from the read functions, will be handled
    // in the WAP and STA classes.
    if (curSrvr == NetMode::WAP || curSrvr == NetMode::WAP_SETUP) {

        // DEF(ault) button, if pressed, will start with the default password.
        // This is to ensure access if for some reason the NVS password is 
        // forgotten.
        bool DEF = gpio_get_level(pinMapD[static_cast<uint8_t>(DPIN::defWAP)]);

        if (DEF) { // Indicates non default mode.

            // shows if the current set pass = the default pass
            if (strcmp(mode.getPass(), mode.getPass(true)) == 0) {
                char tempPass[static_cast<int>(IDXSIZE::PASS)]{0};

                strcpy(tempPass, this->creds.read("WAPpass"));

                // If the NVS pass is not null, sets pass from the NVS.
                if (strcmp(tempPass, "") != 0) {
                    mode.setPass(tempPass);
                } 
            }

        } else { // Default mode
            mode.setPass(mode.getPass(true));
        }

    } else if (curSrvr == NetMode::STA) {
        
        // Ensures that the credentials do not already exist. This is a failsafe
        // in the event that the NVS is corrupt. It allows the credentials to be
        // written from the WAPSetup page into volatile storage only.
        if (strcmp(mode.getPass(), "") == 0) {
            mode.setPass(this->creds.read("pass"));
        }

        if (strcmp(mode.getSSID(), "") == 0) {
            mode.setSSID(this->creds.read("ssid"));
        }

        if (strcmp(mode.getPhone(), "") == 0) {
            mode.setPhone(this->creds.read("phone"));
        }
    }

    // This block ensures that each previous block is properly initialized
    // before it continues to the next function. All error handling is 
    // embedded within these functions.
    if (mode.init_wifi() == wifi_ret_t::INIT_OK) {
        if (mode.start_wifi() == wifi_ret_t::WIFI_OK) {
            if (mode.start_server() == wifi_ret_t::SERVER_OK) {
                mode.mDNS();
            }
        }
    }
}

// All actions are handled here for active connections. This will
// display the current status of the selected connection.
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

    // If not connected, will attempt reconnect. Looks for 
    // an active connection and that all flags have been
    // satisfied.
    if (!mode.isActive()) {
        reconnect(mode, reconAttempt);
    } else {
        reconAttempt = 0;
    }
}

// If connection is inactive, will attempt to start the server, cleaning
// up any pieces that havent been initialized. If all fails after 10 
// attempts, the current mode will be destroyed allowing do-over.
void NetManager::reconnect(NetMain &mode, uint8_t &attempt) {
    mode.sendErr("Reconnecting", errDisp::SRL);
    startServer(mode);
    attempt++;

    if (attempt > 10) {
        mode.destroy();
        attempt = 0;
    }
}

NetManager::NetManager(NetSTA &station, NetWAP &wap, NVS::Creds &creds, UI::Display &OLED) :

    station{station}, wap{wap}, creds(creds), OLED(OLED), isWifiInit(false) {}

void NetManager::handleNet() { 
    NetMode mode = this->checkNetSwitch();

    setNetType(mode);
}







}