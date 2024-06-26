#include "Network/NetworkSTA.h"

namespace Comms {

// The credinfo will be setup upon construction of the object. If changes
// are made, ensure the changes are to the STAcredQty enum.
Station::Station(Messaging::MsgLogHandler &msglogerr) : 

    NetMain(msglogerr), connectedToSTA{false} {

    // Uses CredInfo not from NVS, but the Comms Namespace on the NetworkSTA.h
    // due to requirement of different vars than the NVS offering.
    this->credinfo[0] = {
        NetMain::keys[static_cast<int>(KI::ssid)], NVS::Credentials::getSSID, 
        NetMain::ST_SSID, sizeof(NetMain::ST_SSID)
        };
    this->credinfo[1] = {
        NetMain::keys[static_cast<int>(KI::pass)], NVS::Credentials::getPASS, 
        NetMain::ST_PASS, sizeof(NetMain::ST_PASS)
        };
    this->credinfo[2] = {
        NetMain::keys[static_cast<int>(KI::phone)], NVS::Credentials::getPhone, 
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
void Station::setCredsNVS(NVS::Credentials &Creds) {
    
    if (NetMain::ST_SSID[0] == '\0' || NetMain::ST_PASS[0] == '\0') { 

        // Iterates through credinfo array to gather the required NVS 
        // data upon a fresh connection to the station mode.
        for (const auto &info : this->credinfo) {
            Creds.read(info.key);
            strncpy(info.destination, info.source(), info.size - 1);
            info.destination[info.size - 1] = '\0';
        }
    } 
}

// Station setup mode. Called to connect initially or to reconnect. 
void Station::STAconnect(NVS::Credentials &Creds) {

    // Handle Disconnect
    if (!WiFi.mode(WIFI_OFF)) this->sendErr("Failed to Disconnect AP"); 

    this->setCredsNVS(Creds); // sets the credentials into the system

    if (!WiFi.mode(WIFI_STA)) this->sendErr("WiFi STA mode unsettable"); 

    // Prevents WiFi.begin from multiple calls if it has already began. This 
    // will only be called if it is not connected and there is not an attempt
    // to connect.
    if (WiFi.status() != WL_CONNECTED && WiFi.status() != WL_CONNECT_FAILED) {
        WiFi.begin(NetMain::ST_SSID, NetMain::ST_PASS); 
    }
    
    if (!NetMain::isServerRunning) this->startServer();
}

// If not connected, this will attempt to reconnect at a set interval.
void Station::attemptReconnect(
    NVS::Credentials &Creds, uint8_t freqSec) { 

    uint32_t currentTime{millis()};
    static uint32_t elapsedTime{currentTime};
    
    if ((currentTime - elapsedTime) > freqSec * 1000) {
        STAconnect(Creds); elapsedTime = currentTime;
        this->msglogerr.handle(Levels::CRITICAL, "WiFi Reconnect", Method::OLED);
    }
}

// STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// webpage from any device connected to the LAN. This will also have access to the internet
// for SMS updates and alerts, as well as to check for updated firmware. 
WIFI Station::STA(NVS::Credentials &Creds) {

    // This lambda is responsible for checking the connection and to reconnect if 
    // needed, on top of getting the MDNS up an running. 
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