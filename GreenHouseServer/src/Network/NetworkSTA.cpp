#include "Network/NetworkSTA.h"

namespace Comms {

Station::Station(Messaging::MsgLogHandler &msglogerr) : 
    NetMain(msglogerr), connectedToSTA{false} {}

// Sends station exclusive details to OLED.
STAdetails Station::getSTADetails() {
    STAdetails details{"", "", ""};
    strncpy(details.SSID, WiFi.SSID().c_str(), sizeof(details.SSID) - 1); 
    details.SSID[sizeof(details.SSID) - 1] = '\0';
    IPAddress ip{WiFi.localIP()};
    sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());
    return details;
}   

// Connection to the station mode.
void Station::STAconnect(FlashWrite::Credentials &Creds) {
    memset(this->error, 0, sizeof(this->error));
    uint8_t errCt{0};

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) {
            appendErr("Failed to Disconnect AP; "); errCt++;
        }

    // Checks if SSID/PASS have already been created on the WAP Setup page, which
    // will serve as a redundancy to allow station access if the NVS fails.
    // If these dont exist, typical for a fresh start, the NVS will be read
    // to provide credentials. If nothing exists, there will be no SSID or 
    // PASS to connect to station.
    if (NetMain::ST_SSID[0] == '\0' || NetMain::ST_PASS[0] == '\0') {

        Creds.read("ssid"); Creds.read("pass"); Creds.read("phone");

        // Copy the read data to the class variables to make them a primary
        // means of credential entry.
        strncpy(NetMain::ST_SSID, Creds.getSSID(), sizeof(NetMain::ST_SSID) - 1);
        NetMain::ST_SSID[sizeof(NetMain::ST_SSID) - 1] = '\0'; 

        strncpy(NetMain::ST_PASS, Creds.getPASS(), sizeof(NetMain::ST_PASS) - 1);
        NetMain::ST_PASS[sizeof(NetMain::ST_PASS) - 1] = '\0';

        strncpy(NetMain::phone, Creds.getPhone(), sizeof(NetMain::phone) - 1);
        NetMain::phone[sizeof(NetMain::phone) - 1] = '\0';
    } 

    if (!WiFi.mode(WIFI_STA)) {
        appendErr("WiFi STA mode unsettable; "); errCt++;
    }

    if (errCt > 0) {
        this->msglogerr.handle(
            Levels::ERROR, this->error, Method::OLED, Method::SRL);
    }  

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