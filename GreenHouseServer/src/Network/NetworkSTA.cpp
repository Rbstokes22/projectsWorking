#include "Network/NetworkSTA.h"

namespace Comms {

Station::Station(Messaging::MsgLogHandler &msglogerr) : 

    NetMain(msglogerr), connectedToSTA{false} {

    credsinfo[0] = {
        "ssid", FlashWrite::Credentials::getSSID, 
        NetMain::ST_SSID, sizeof(NetMain::ST_SSID)
        };
    credsinfo[1] = {
        "pass", FlashWrite::Credentials::getPASS, 
        NetMain::ST_PASS, sizeof(NetMain::ST_PASS)
        };
    credsinfo[2] = {
        "phone", FlashWrite::Credentials::getPhone, 
        NetMain::phone, sizeof(NetMain::phone)
        };
    }

// Called at a set interval whenever the status of Station is checked. This sends
// the current SSID, IP, and Signal Strength over to the OLED since just those
// items are exclusive to the station connection.
STAdetails Station::getSTADetails() {
    STAdetails details{"", "", ""};
    
    strncpy(details.SSID, WiFi.SSID().c_str(), sizeof(details.SSID) - 1); 
    details.SSID[sizeof(details.SSID) - 1] = '\0';

    IPAddress ip{WiFi.localIP()};
    sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());

    return details;
}   

// Checks if SSID/PASS have already been created on the WAP Setup page, which
// will serve as a redundancy to allow station access if the NVS fails.
// If these dont exist, typical for a fresh start, the NVS will be read
// to provide credentials. If nothing exists, there will be no SSID or 
// PASS to connect to station.
void Station::setCredsNVS(FlashWrite::Credentials &Creds) {
    
    if (NetMain::ST_SSID[0] == '\0' || NetMain::ST_PASS[0] == '\0') { 

        for (const auto &info : credsinfo) {
            Creds.read(info.key);
            strncpy(info.destination, info.source(), info.size - 1);
            info.destination[info.size - 1] = '\0';
        }
    } 
}

// Station setup mode. Called to connect initiall or to reconnect. 
void Station::STAconnect(FlashWrite::Credentials &Creds) {

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) this->sendErr("Failed to Disconnect AP"); 

    this->setCredsNVS(Creds);

    if (!WiFi.mode(WIFI_STA)) this->sendErr("WiFi STA mode unsettable"); 

    // Does not call begin if there is already an attempt or active connection
    if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) {
        WiFi.begin(NetMain::ST_SSID, NetMain::ST_PASS);
    }
    
    if (!NetMain::isServerRunning) this->startServer();
}

// If not connected, this will attempt reconnect
void Station::attemptReconnect(
    FlashWrite::Credentials &Creds, uint8_t freqSec) { // default 10

    static uint64_t elapsedTime = millis();

    if ((millis() - elapsedTime) > freqSec * 1000) {
        STAconnect(Creds); elapsedTime = millis();
    }
}

// STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// webpage from any device connected to the LAN. This will also have access to the internet
// for SMS updates and alerts, as well as to check for updated firmware. 
WIFI Station::STA(FlashWrite::Credentials &Creds) {

    auto manageConnection = [this, &Creds](bool reconnect = false) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!MDNSrunning) {MDNS.begin("esp32"); MDNSrunning = true;}
            this->connectedToSTA = true;
            return WIFI::WIFI_RUNNING;
        } else {
            this->connectedToSTA = false;
            if (reconnect) attemptReconnect(Creds, 10);
            return WIFI::WIFI_STARTING;
        }
    };
    
    if (WiFi.getMode() != WIFI_STA || NetMain::prevServerType != WIFI::STA_ONLY) {
        STAconnect(Creds);

        // This must follow connection for purposes of handling previous 
        // server disconnections
        NetMain::prevServerType = WIFI::STA_ONLY;
        return manageConnection();

    } else {
        // This is essential to the non-blocking WiFi begin instead of
        // waiting for delay. Will attempt reconnect if it is disconnected
        // in this block.
        return manageConnection(true);
    }
}

}