#include "Network/NetSTA.hpp"
#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include "UI/MsgLogHandler.hpp"
#include <cstdint>
#include "Network/Routes.hpp"
#include "string.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mdns.h"
#include <cstddef>

namespace Comms {

// STATIC

// handled within IP event register.
char NetSTA::IPADDR[static_cast<int>(IDXSIZE::IPADDR)]{0};

// PRIVATE

// Sets the wifi configuration with the station ssid and 
// password. Returns CONFIG_OK once complete.
wifi_ret_t NetSTA::configure() { 

    strcpy((char*)this->wifi_config.sta.ssid, this->ssid);
    strcpy((char*)this->wifi_config.sta.password, this->pass); 

    return wifi_ret_t::CONFIG_OK;
}

// This IP event is a registered event that copies the assigned
// IP addr into the IPADDR char array.
void NetSTA::IPEvent(
    void* arg, 
    esp_event_base_t eventBase, 
    int32_t eventID, 
    void* eventData) {

    if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
        esp_netif_ip_info_t ip_info = event->ip_info;

        esp_ip4addr_ntoa(&ip_info.ip, NetSTA::IPADDR, sizeof(NetSTA::IPADDR));
    }
}

// PUBLIC

NetSTA::NetSTA(const char* mdnsName) : NetMain(mdnsName) {

    memset(this->ssid, 0, sizeof(this->ssid));
    memset(this->pass, 0, sizeof(this->pass));
}

// Second step in the init process.
// Once the wifi has been initialized, this will register the 
// IP event handler, configure the wifi connection, set the wifi
// mode, set the configuration, start the wifi, and connect to the
// station. Returns WIFI_FAIL and WIFI_OK.
wifi_ret_t NetSTA::start_wifi() {

    if (!NetMain::flags.getFlag(handlerReg)) {
        
        esp_err_t handler = esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent, NULL);

        if (handler == ESP_OK) {
            NetMain::flags.setFlag(handlerReg);
        } else {
            this->sendErr("Handler not registered");
            this->sendErr(esp_err_to_name(handler));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Always run, no flag.
    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL; // error prints in configure
    } 

    if (!NetMain::flags.getFlag(wifiModeSet)) {
        
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_STA);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.setFlag(wifiModeSet);
        } else {
            this->sendErr("Wifi mode not set");
            this->sendErr(esp_err_to_name(wifi_mode));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(wifiConfigSet)) {
        
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_STA, &this->wifi_config);

        if (wifi_cfg == ESP_OK) {
            NetMain::flags.setFlag(wifiConfigSet);
        } else {
            this->sendErr("Wifi config not set");
            this->sendErr(esp_err_to_name(wifi_cfg));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(wifiOn)) {
        
        esp_err_t wifi_start = esp_wifi_start();

        if (wifi_start == ESP_OK) {
            NetMain::flags.setFlag(wifiOn);
        } else {
            this->sendErr("Wifi not started");
            this->sendErr(esp_err_to_name(wifi_start));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(wifiConnected)) {
        
        esp_err_t wifi_con = esp_wifi_connect();

        if (wifi_con == ESP_OK) {
            NetMain::flags.setFlag(wifiConnected);
        } else {
            this->sendErr("Wifi not connected");
            this->sendErr(esp_err_to_name(wifi_con));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

// Third step of init process.
// Once the wifi connection is established, starts the httpd
// server and registers the Universal Resource Identifiers
// (URI). Returns SERVER_FAIL or SERVER_OK.
wifi_ret_t NetSTA::start_server() {

    if (!NetMain::flags.getFlag(httpdOn)) {
        
        esp_err_t httpd = httpd_start(&NetMain::server, &NetMain::http_config);

        if (httpd == ESP_OK) {
                NetMain::flags.setFlag(httpdOn);
        } else {
            this->sendErr("HTTP not started");
            this->sendErr(esp_err_to_name(httpd));
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(uriReg)) {
        
        esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, &STAIndex);
        esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, &OTAUpdate);
        esp_err_t reg3 = httpd_register_uri_handler(NetMain::server, &OTARollback);
        esp_err_t reg4 = httpd_register_uri_handler(NetMain::server, &OTACheck);
        esp_err_t reg5 = httpd_register_uri_handler(NetMain::server, &OTAUpdateLAN);
        esp_err_t reg6 = httpd_register_uri_handler(NetMain::server, &ws);
        esp_err_t reg7 = httpd_register_uri_handler(NetMain::server, &log);

        if (reg1 == ESP_OK && reg2 == ESP_OK && reg3 == ESP_OK && reg4 == ESP_OK
            && reg5 == ESP_OK && reg6 == ESP_OK && reg7 == ESP_OK) {
                
            NetMain::flags.setFlag(uriReg);
        } else {
            this->sendErr("STA URI's unregistered");
            this->sendErr(esp_err_to_name(reg1));
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    return wifi_ret_t::SERVER_OK;
}

// Stops the httpd server and wifi, disconnects from the station,
// and unregisters the IP handler. This will reset all pertinent 
// flags back to false, to allow reinitialization through the 
// init sequence. Returns DESTROY_FAIL or DESTROY_OK.
wifi_ret_t NetSTA::destroy() {

    if (NetMain::flags.getFlag(mdnsInit)) {
        
        mdns_free();
        NetMain::flags.releaseFlag(mdnsInit);
        NetMain::flags.releaseFlag(mdnsHostSet);
        NetMain::flags.releaseFlag(mdnsInstanceSet);
        NetMain::flags.releaseFlag(mdnsServiceAdded);
    }

    if (NetMain::flags.getFlag(httpdOn)) {
        
        esp_err_t httpd = httpd_stop(NetMain::server);

        if (httpd == ESP_OK) {
            NetMain::flags.releaseFlag(httpdOn);
            NetMain::flags.releaseFlag(uriReg);
        } else {
            this->sendErr("HTTPD not stopped");
            this->sendErr(esp_err_to_name(httpd));
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.getFlag(wifiOn)) {
        
        esp_err_t wifi_stop = esp_wifi_stop();

        if (wifi_stop == ESP_OK) {
            NetMain::flags.releaseFlag(wifiOn);
            NetMain::flags.releaseFlag(wifiModeSet);
            NetMain::flags.releaseFlag(wifiConfigSet);
        } else {
            this->sendErr("Wifi not stopped");
            this->sendErr(esp_err_to_name(wifi_stop));
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.getFlag(wifiConnected)) {
        
        esp_err_t wifi_con = esp_wifi_disconnect();

        if (wifi_con == ESP_OK || wifi_con == ESP_ERR_WIFI_NOT_STARTED) {
            NetMain::flags.releaseFlag(wifiConnected);
        } else {
            this->sendErr("Wifi not disconnected");
            this->sendErr(esp_err_to_name(wifi_con));
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.getFlag(handlerReg)) {
        
        esp_err_t event = esp_event_handler_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent);

        if (event == ESP_OK) {
            NetMain::flags.releaseFlag(handlerReg);
            
        } else {
            this->sendErr("Wifi handler not unregistered");
            this->sendErr(esp_err_to_name(event));
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    return wifi_ret_t::DESTROY_OK;
}

// Sets the password. Max pass length 63 chars.
void NetSTA::setPass(const char* pass) {
    if (strlen(pass) != 0) {
        strncpy(this->pass, pass, sizeof(this->pass) -1);
        this->pass[sizeof(this->pass) - 1] = '\0';
    } 
}

// Sets the ssid. Max ssid length is 32 chars.
void NetSTA::setSSID(const char* ssid) {
    if (strlen(ssid) != 0) {
        strncpy(this->ssid, ssid, sizeof(this->ssid) -1);
        this->ssid[sizeof(this->ssid) - 1] = '\0';
    } 
}

const char* NetSTA::getPass(bool def) const {
    return this->pass;
}

const char* NetSTA::getSSID() const {
    return this->ssid;
}

// Runs an iteration of all flags pertaining to the station 
// connection. Upon all flags being set and the station 
// being connected, returns true. Returns false for failure.
bool NetSTA::isActive() {
    wifi_ap_record_t sta_info;

    uint8_t flagRequirement = 15;
    uint8_t flagSuccess = 0;

    bool required[flagRequirement]{
        NetMain::flags.getFlag(wifiInit), 
        NetMain::flags.getFlag(netifInit),
        NetMain::flags.getFlag(eventLoopInit), 
        NetMain::flags.getFlag(sta_netifCreated),
        NetMain::flags.getFlag(wifiModeSet), 
        NetMain::flags.getFlag(wifiConfigSet),
        NetMain::flags.getFlag(wifiOn), 
        NetMain::flags.getFlag(httpdOn),
        NetMain::flags.getFlag(uriReg), 
        NetMain::flags.getFlag(handlerReg),
        NetMain::flags.getFlag(wifiConnected), 
        NetMain::flags.getFlag(mdnsInit),
        NetMain::flags.getFlag(mdnsHostSet), 
        NetMain::flags.getFlag(mdnsInstanceSet),
        NetMain::flags.getFlag(mdnsServiceAdded)
    };

    for (int i = 0; i < flagRequirement; i++) {
        if (required[i] == true) flagSuccess++;
    }

    if (flagSuccess != flagRequirement) {
        this->sendErr("Wifi flags not met");
        return false;
    } else if (esp_wifi_sta_get_ap_info(&sta_info) == ESP_OK) {
        return true;
    } 

    return false;
}

// Creates a struct containing the ssid, ipaddr, rssi,
// connection status, and free memory. The struct is passed
// by reference and updated.
void NetSTA::getDetails(STAdetails &details) { 
    wifi_ap_record_t sta_info;
    
    char status[2][4] = {"No", "Yes"}; // used to display connection

    // SSID
    if (esp_wifi_sta_get_ap_info(&sta_info) != ESP_OK) {
        strcpy(details.ssid, "Not Connected");
    } else {
        strcpy(details.ssid, reinterpret_cast<const char*>(sta_info.ssid));
    }

    // IP
    snprintf(
        details.ipaddr, 
        sizeof(details.ipaddr),
        "http://%s", 
        NetSTA::IPADDR
        );

    // MDNS
    char hostname[static_cast<int>(IDXSIZE::MDNS)];
    mdns_hostname_get(hostname);
    snprintf(
        details.mdns, 
        sizeof(details.mdns), 
        "http://%s.local", 
        hostname
        );

    // STATUS
    strcpy(details.status, status[this->isActive()]);
    
    // RSSI signal strength 
    sprintf(details.signalStrength, "%d dBm", sta_info.rssi);

    // REMAINING HEAP SIZE
    sprintf(details.heap, "%.2f KB", this->getHeapSize(HEAP_SIZE::KB));
}

}