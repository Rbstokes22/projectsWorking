#include "Network/NetManager.hpp"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "driver/gpio.h"
#include "Config/config.hpp"
#include "UI/Display.hpp"
#include "UI/MsgLogHandler.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Peripherals/saveSettings.hpp" // Used for destruction fail reset

namespace Comms {

// Requires no params. Checks reading of the GPIO pins to determine if the
// switch is in station, WAP setup, or WAP mode. Returns STA, WAP, or WAP_SETUP.
NetMode NetManager::checkNetSwitch() {
    bool WAP = gpio_get_level(
        CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::WAP)]);

    bool STA = gpio_get_level(
        CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::STA)]);

    // activation means dropped down to ground.
    if (WAP && !STA) {return NetMode::STA;}
    else if (!WAP && STA) {return NetMode::WAP;}
    else if (WAP && STA) {return NetMode::WAP_SETUP;}
    else {return NetMode::NONE;}
}

// Requires NetMode NetType param Determines if the set position is that of a 
// WAP or STA. Checks connection based on this type.
void NetManager::setNetType(NetMode NetType) {

    if (NetType == NetMode::WAP || NetType == NetMode::WAP_SETUP) {
        this->checkConnection(this->wap, NetType);
    } else if (NetType == NetMode::STA) {
        this->checkConnection(this->station, NetType);
    }
}

// Requires the mode (STA or WAP), and the Network Type. Checks that type 
// against the current switch position. Destroys current connection and 
// switches to the new connection. If connection is already present, executes
// the running wifi method instead.
void NetManager::checkConnection(NetMain &mode, NetMode NetType) {
    NetMode net_type = NetMain::getNetType();
    static const char* modes[] = {"WAP", "WAP_SETUP", "STA"}; // logging use.
    
    if (NetType != net_type) { // Checks if the mode has been switched

        // WAP_RECON will be flagged upon a change of the WAP password. 
        // This will allow a new connection using new creds.
        if (net_type == NetMode::WAP || net_type == NetMode::WAP_SETUP || 
            net_type == NetMode::WAP_RECON) {

            // If non-resseting due to fail, returns to block code. Recommend
            // enabling NET_DESTROY_FAIL_FORCE_RESET to true.
            if (!this->handleDestruction(this->wap)) return;

            // Handles logging for connection destruction. WAP class will 
            // log for new/active connection.
            snprintf(this->log, sizeof(this->log), "%s: WAP connect destroyed", 
                this->tag);

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
                this->log, Messaging::Method::SRL_LOG);

        } else if (net_type == NetMode::STA) {

            // Same as WAP.
            if (!this->handleDestruction(this->station)) return;
   
            // Handles logging for connection destruction. STA class will 
            // log for new/active connection.
            snprintf(this->log, sizeof(this->log), "%s: STA connect destroyed", 
                this->tag);

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
                this->log, Messaging::Method::SRL_LOG);
        } 
    
        // startServer() will be called exactly one time before the 
        // program goes into runningWifi() which assumes a solid
        // init. In the event this fails, startServer() is also called 
        // when attempting a reconnection in other methods to handle redundancy.
        mode.setNetType(NetType);

        snprintf(this->log, sizeof(this->log), "%s: Starting server mode %s", 
            this->tag, modes[static_cast<uint8_t>(NetType)]);

        this->startServer(mode);
    }

    else {
        this->runningWifi(mode);
    }
}

// Requires the mode (STA or WAP). NVS credentials are checked. Default values
// will be used if the data is invalid, which means the WAP will start but STA
// will not. If the default button is held upon the startup, it will start with
// the default password.
void NetManager::startServer(NetMain &mode) { 
    NetMode curSrvr = NetMain::getNetType();
    NVS::Creds* creds = NVS::Creds::get();
    
    // All logic depending on returns from the read functions, will be handled
    // in the WAP and STA subclasses classes.

    // Checks current server mode.
    if (curSrvr == NetMode::WAP || curSrvr == NetMode::WAP_SETUP) {

        // DEF(ault) button, if pressed, will start with the default password.
        // This is to ensure access if for some reason the NVS password is 
        // forgotten.
        bool nDEF = gpio_get_level(
            CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::defWAP)]);

        if (nDEF) { // Indicates non-default mode.

            // shows if the current set pass = the default pass. getPass(true)
            // means return the default password.
            if (strcmp(mode.getPass(), mode.getPass(true)) == 0) {
                char tempPass[static_cast<int>(IDXSIZE::PASS)]{0};

                strcpy(tempPass, creds->read("WAPpass")); // copy NVS to actual.

                // If the NVS pass is not empty, sets pass from the NVS. If
                // empty, nothing changes and default pass is kept.
                if (strcmp(tempPass, "") != 0) {
                    mode.setPass(tempPass);
                } 
            }

        } else { // Default mode
            mode.setPass(mode.getPass(true));
        }

    } else if (curSrvr == NetMode::STA) { // Checks station.
        
        // Ensures that the credentials do not already exist. This is a failsafe
        // in the event that the NVS is corrupt. It allows the credentials to be
        // written from the WAPSetup page into volatile storage only for the
        // duration of the program.
        if (strcmp(mode.getPass(), "") == 0) {

            // Sets password from NVS upon first init only, since init = ""
            mode.setPass(creds->read("pass")); 
        }

        if (strcmp(mode.getSSID(), "") == 0) {

            // Sets ssid from NVS upon first init only, since init = ""
            mode.setSSID(creds->read("ssid"));
        }
    }

    // This block ensures that each previous block is properly initialized
    // before it continues to the next function. All error handling is 
    // embedded within these functions.
    if (mode.init_wifi() == wifi_ret_t::INIT_OK) {
        if (mode.start_wifi() == wifi_ret_t::WIFI_OK) {
            if (mode.start_server() == wifi_ret_t::SERVER_OK) {
                mode.mDNS(); // Final set for running server.
            }
        }
    }
}

// Requires mode (STA or WAP). Handles active connections only. Handles events
// where reconnection is necessary, as well as prints to OLED, current network
// status.
void NetManager::runningWifi(NetMain &mode) {
    NetMode curSrvr = NetMain::getNetType();
    static uint8_t reconAttempt{0}; // Reconnection attempt

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
    if (!mode.isActive()) { // handle activity logging in the subclasses.
        reconnect(mode, reconAttempt); // handle logging in the subclasses.
    } else {
        reconAttempt = 0;
    }
}

// Requires mode (WAP or STA) and a reference to the attempt since it is 
// incremented here. If connection is inactive, attempts starting server, 
// cleaning up any pieces that have not been init. If attempts exceed the
// max, destroys current mode and restarts.
void NetManager::reconnect(NetMain &mode, uint8_t &attempt) {
    static bool sentLog = false;

    if (!sentLog) { // Sends log only once per reattempt session.
        sentLog = true;
        snprintf(this->log, sizeof(this->log), "%s: Net Reconnecting", 
            this->tag);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
            this->log, Messaging::Method::SRL_LOG);
    } 

    // Sends via SRL every reconnect attempt. This allows pollution control to
    // the log.
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::WARNING,
        this->log, Messaging::Method::SRL);

    startServer(mode);
    attempt++;

    if (attempt > NET_ATTEMPTS_RECON) {
        mode.destroy();
        attempt = 0;
        sentLog = false; // Resets to allow relogging
    }
}

// Requires the mode (STA or WAP). Attempts to destroy the connection, and if
// attempts are exceeded, will return false or reset depeding on the 
// NET_DESTROY_FAIL_FORCE_RESET setting. Returns true upon success.
bool NetManager::handleDestruction(NetMain &mode) {
    uint8_t destroyAttempt = 0;

    while (this->wap.destroy() != wifi_ret_t::DESTROY_OK) {
            
        // Check for attempt. If this occurs, chances are that the
        // device will need to be reset, considering destroying the
        // connection is brute.
        if (++destroyAttempt >= NET_ATTEMPTS_DESTROY) {

            // Handle error messaging immediately before action.
            snprintf(this->log, sizeof(this->log), 
                "%s: Connection unable to destroy", this->tag);

            Messaging::MsgLogHandler::get()->handle(
                Messaging::Levels::CRITICAL, this->log, 
                Messaging::Method::SRL_LOG);

            // if true, will save settings and force a reset.
            if (NET_DESTROY_FAIL_FORCE_RESET) {
                NVS::settingSaver::get()->save();
                vTaskDelay(pdMS_TO_TICKS(10)); // Brief delay
                esp_restart();
            }

            // If false, returns false and resets attempts. DO NOT RECOMMEND.
            
            destroyAttempt = 0; // reset
            return false; // Block remaining code.
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // brief delay between loops.
    }

    destroyAttempt = 0; // reset
    return true;
}

// Constructor. Takes station, wap, and OLED references.
NetManager::NetManager(
    NetSTA &station, 
    NetWAP &wap, 
    UI::Display &OLED) :

    tag("NetMan"), station{station}, wap{wap}, OLED(OLED), isWifiInit(false) {

        memset(this->log, 0, sizeof(this->log));
    }

// Requires no params. This is the first part of the process and checks the 
// position of the netswitch, and sends the mode to be handled.
void NetManager::handleNet() { 
    NetMode mode = this->checkNetSwitch();
    setNetType(mode);
}

}