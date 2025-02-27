#include "Network/NetMain.hpp"
#include "Network/NetWAP.hpp"
#include "esp_wifi.h"
#include <cstdint>
#include "Network/Routes.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"
#include "lwip/inet.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mdns.h"
#include <cstddef>

namespace Comms {

// Requires no parameters. Sets the wireless access point configuration with
// the ssid, password, channel, maximum connections, and authentication mode.
// Uses max connection default of 1 while in WAP Setup mode as an extra layer
// of security since the communications are on http. This will attempt to limit
// malicious connections from intercepting data. Returns CONFIG_OK.
wifi_ret_t NetWAP::configure() {
    uint8_t maxConnections{WAP_MAX_CONNECTIONS};

    // Set max connection to 1 if in WAP setup mode.
    if (NetMain::NetType == NetMode::WAP_SETUP) maxConnections = 1;

    // Copy and set all pertinent data to include the ssid, pass, length of
    // each, max connections, channel, and auth mode.
    strcpy((char*)this->wifi_config.ap.ssid, this->APssid);
    this->wifi_config.ap.ssid_len = strlen(this->APssid);
    this->wifi_config.ap.channel = 6;
    strcpy((char*)this->wifi_config.ap.password, this->APpass);
    this->wifi_config.ap.max_connection = maxConnections;
    this->wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    
    // If password is under 8 chars, it will be a password-less open mode.
    if (strlen(this->APpass) < 8) {
        this->sendErr("Open Network due to pass lgth");
        this->wifi_config.ap.authmode = WIFI_AUTH_OPEN; 
    }

    return wifi_ret_t::CONFIG_OK;
}

// Requires no parameters. Dynamic Host Configuration Protocol (DHCP) will stop
// to allow IP information to be configured to the device for the WAP
// connection. The IP information is set, which will allow the DHCP to restart
// returning DHCPS_FAIL or DHCPS_OK.
wifi_ret_t NetWAP::dhcpsHandler() {

    static bool isInit = false; // used for initial startup ONLY

    if (!isInit) { // Stop dhcps upon first init to minimize error.

        // dhcp flag will already be off when starting initially.
        esp_err_t stop = esp_netif_dhcps_stop(NetMain::ap_netif); 

        if (stop != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED &&
            stop != ESP_OK) {

            snprintf(NetMain::errlog, sizeof(NetMain::errlog), 
                "%s: DHCPS not stopped: %s", this->tag, esp_err_to_name(stop));
            
            this->sendErr(NetMain::errlog);
            return wifi_ret_t::DHCPS_FAIL;
            }

        // Setup the ip, gateway, and netmask. This allows (255 - 2) Addresses.
        IP4_ADDR(&this->ip_info.ip, 192,168,1,1); 
        IP4_ADDR(&this->ip_info.gw, 192,168,1,1);
        IP4_ADDR(&this->ip_info.netmask, 255, 255, 255, 0);
        isInit = true; // Sets to true upon success and continutes.
    }
    
    // Handles a reset, when called if flag is on, will stop dhcps server
    // and release the flag.
    if (NetMain::flags.getFlag(NETFLAGS::dhcpOn)) { 
        
        esp_err_t stop = esp_netif_dhcps_stop(NetMain::ap_netif);
        
        if (stop == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED || 
            stop == ESP_OK) {

            NetMain::flags.releaseFlag(dhcpOn);

        } else {
            snprintf(NetMain::errlog, sizeof(NetMain::errlog), 
                "%s: DHCPS not stopped: %s", this->tag, esp_err_to_name(stop));
            
            this->sendErr(NetMain::errlog);
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    // START HERE. LOOK AT THE FLOW OF THIS, MAKE CHANGES AND TEST. THIS LOOKS
    // TO BE PRONE TO ERRORS CONSIDERING THE FLAGS WITHIN FLAGS, THIS COULD
    // PREVENT SOMETHING FROM BEING SET DUE TO PRE-MATURE BLOCKING OR SOMETHING.
    if (!NetMain::flags.getFlag(NETFLAGS::dhcpOn)) {

        if (!NetMain::flags.getFlag(NETFLAGS::dhcpIPset)) {
            esp_err_t set_ip = esp_netif_set_ip_info(NetMain::ap_netif, 
                &this->ip_info);

            // This will force a stop if this error is returned, 
            // allowing a proper initialization.
            if (set_ip == ESP_ERR_ESP_NETIF_DHCP_NOT_STOPPED) {
                NetMain::flags.setFlag(NETFLAGS::dhcpOn); // indicates still on
            } 
            
            // no else if, seperate statement to allow else block to invoked
            if (set_ip == ESP_OK) { // Separate to setting a diff flag.
                    NetMain::flags.setFlag(NETFLAGS::dhcpIPset);
            } else { // If any failure return including DHCP not stopped.
                this->sendErr("IP info not set");
                this->sendErr(esp_err_to_name(set_ip));
                return wifi_ret_t::DHCPS_FAIL;
            }
        }

        esp_err_t start = esp_netif_dhcps_start(NetMain::ap_netif);

        if (start == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED || 
            start == ESP_OK) {

            NetMain::flags.setFlag(NETFLAGS::dhcpOn);

        } else {
            this->sendErr("DHCPS not started");
            this->sendErr(esp_err_to_name(start));
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    return wifi_ret_t::DHCPS_OK;
}

// PUBLIC

NetWAP::NetWAP(const char* APssid, const char* APdefPass, 
    const char* mdnsName) : NetMain(mdnsName), tag("NETWAP") {

    strncpy(this->APssid, APssid, sizeof(this->APssid) - 1);
    this->APssid[sizeof(this->APssid) - 1] = '\0';

    strncpy(this->APpass, APdefPass, sizeof(this->APpass) - 1);
    this->APpass[sizeof(this->APpass) -1] = '\0';

    strcpy(this->APdefaultPass, APdefPass);
}

// Second step in the init process.
// Once the wifi has been initialized, run the dhcps handler
// to set the IP, configure the wifi connection, set the 
// wifi mode, set the configuration, and start the wifi.
// Returns WIFI_FAIL and WIFI_OK.
wifi_ret_t NetWAP::start_wifi() {

    // Always run, flags are withing the handler.
    if (this->dhcpsHandler() == wifi_ret_t::DHCPS_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    // Always run, no flag.
    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    if (!NetMain::flags.getFlag(NETFLAGS::wifiModeSet)) {
        
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_AP);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiModeSet);
        } else {
            this->sendErr("Wifi mode not set");
            this->sendErr(esp_err_to_name(wifi_mode));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(NETFLAGS::wifiConfigSet)) {
        
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_AP, &this->wifi_config);

        if (wifi_cfg == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiConfigSet);
        } else {
            this->sendErr("Wifi config not set");
            this->sendErr(esp_err_to_name(wifi_cfg));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_start = esp_wifi_start();

        if (wifi_start == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiOn);
        } else {
            this->sendErr("Wifi not started");
            this->sendErr(esp_err_to_name(wifi_start));
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

// Third step of init process.
// Once the wifi connection is established, starts the httpd
// server and registers the Universal Resource Identifiers
// (URI). Returns SERVER_FAIL or SERVER_OK.
wifi_ret_t NetWAP::start_server() {

    if (!NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
        esp_err_t httpd = httpd_start(&NetMain::server, &NetMain::http_config);

        if (httpd == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::httpdOn);
        } else {
            this->sendErr("HTTP not started");
            this->sendErr(esp_err_to_name(httpd));
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    if (!NetMain::flags.getFlag(NETFLAGS::uriReg)) {
        
        // This will register different URI's based on the type of WAP
        // that is set. This allows each version to use their own index
        // page, and avoids the need for complex URL's.
        if (NetMain::NetType == NetMode::WAP) {
            
            esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, &WAPIndex);
            esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, &ws);

            if (reg1 == ESP_OK && reg2 == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::uriReg);
                return wifi_ret_t::SERVER_OK;
            } else {
                this->sendErr("WAP URI's unregistered");
                this->sendErr(esp_err_to_name(reg1));
                return wifi_ret_t::SERVER_FAIL;
            }

        } else if (NetMain::NetType == NetMode::WAP_SETUP) {
            esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, &WAPSetupIndex);
            esp_err_t reg3 = httpd_register_uri_handler(NetMain::server, &WAPSubmitCreds);

            if (reg2 == ESP_OK && reg3 == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::uriReg);
            } else {
                this->sendErr("WAPSetup URI's unregistered");
                this->sendErr(esp_err_to_name(reg2));
                this->sendErr(esp_err_to_name(reg3));
                return wifi_ret_t::SERVER_FAIL;
            }
        } 
    }

    return wifi_ret_t::SERVER_OK;
}
       
// Stops the httpd server and wifi. This will reset all
// pertinent flags back to false, to allow reinitialization
// through the init sequence. Returns DESTROY_FAIL and DESTROY_OK.
wifi_ret_t NetWAP::destroy() { 

    if (NetMain::flags.getFlag(NETFLAGS::mdnsInit)) {
        
        mdns_free();
        NetMain::flags.releaseFlag(mdnsInit);
        NetMain::flags.releaseFlag(mdnsHostSet);
        NetMain::flags.releaseFlag(mdnsInstanceSet);
        NetMain::flags.releaseFlag(mdnsServiceAdded);
    }

     if (NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
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

    if (NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_stop = esp_wifi_stop();

        if (wifi_stop == ESP_OK) {
            NetMain::flags.releaseFlag(wifiOn);
            NetMain::flags.releaseFlag(wifiModeSet);
            NetMain::flags.releaseFlag(dhcpIPset);
            NetMain::flags.releaseFlag(wifiConfigSet);
            
        } else {
            this->sendErr("Wifi not stopped");
            this->sendErr(esp_err_to_name(wifi_stop));
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    return wifi_ret_t::DESTROY_OK;
}

// Sets the password. Max pass length 63 chars.
void NetWAP::setPass(const char* pass) {
    if (strlen(pass) != 0) {
        strncpy(this->APpass, pass, sizeof(this->APpass) -1);
        this->APpass[sizeof(this->APpass) - 1] = '\0';
    } 
}

const char* NetWAP::getPass(bool defaultPass) const {
    if (!defaultPass) {
        return this->APpass;
    } else {
        return this->APdefaultPass;
    }
}

// Abstract methods not needed in the scope of NetWAP.
void NetWAP::setSSID(const char* ssid) {}
const char* NetWAP::getSSID() const {return "";}

// Runs an iteration of all flags pertaining to the station 
// connection. Upon all flags being set and the station 
// being connected, returns true. Returns false for failure.
bool NetWAP::isActive() { 
    wifi_mode_t mode;
    uint8_t flagRequirement = 15;
    uint8_t flagSuccess = 0;

    bool required[flagRequirement]{
        NetMain::flags.getFlag(NETFLAGS::wifiInit), 
        NetMain::flags.getFlag(NETFLAGS::netifInit),
        NetMain::flags.getFlag(NETFLAGS::eventLoopInit), 
        NetMain::flags.getFlag(NETFLAGS::ap_netifCreated),
        NetMain::flags.getFlag(NETFLAGS::dhcpOn), 
        NetMain::flags.getFlag(NETFLAGS::dhcpIPset),
        NetMain::flags.getFlag(NETFLAGS::wifiModeSet), 
        NetMain::flags.getFlag(NETFLAGS::wifiConfigSet),
        NetMain::flags.getFlag(NETFLAGS::wifiOn), 
        NetMain::flags.getFlag(NETFLAGS::httpdOn),
        NetMain::flags.getFlag(NETFLAGS::uriReg), 
        NetMain::flags.getFlag(NETFLAGS::mdnsInit),
        NetMain::flags.getFlag(NETFLAGS::mdnsHostSet), 
        NetMain::flags.getFlag(NETFLAGS::mdnsInstanceSet),
        NetMain::flags.getFlag(NETFLAGS::mdnsServiceAdded)
    };

    for (int i = 0; i < flagRequirement; i++) {
        if (required[i] == true) flagSuccess++;
    }

    if (flagSuccess != flagRequirement) {
        this->sendErr("Wifi flags not met");
        return false;
    } else if (esp_wifi_get_mode(&mode) == ESP_OK && mode == WIFI_MODE_AP) {
        wifi_config_t ap_config;
        if (esp_wifi_get_config(WIFI_IF_AP, &ap_config) == ESP_OK) {
            return true;
        }
    }

    return false;
}

// Creates a struct containing the ipaddr, connection status,
// the WAP type (WAP or WAP_SETUP), free memory, and clients
// connected. The struct is passed by reference and updated.
void NetWAP::getDetails(WAPdetails &details) {

    char status[2][4] = {"No", "Yes"}; // used to display connection
    char WAPtype[20]{0};

    // IPADDR & CONNECTION STATUS
    snprintf(
        details.ipaddr,
        sizeof(details.ipaddr),
        "http://192.168.1.1"
        );

    strcpy(details.status, status[this->isActive()]);

    // MDNS
    char hostname[static_cast<int>(IDXSIZE::MDNS)];
    mdns_hostname_get(hostname);
    snprintf(
        details.mdns, 
        sizeof(details.mdns), 
        "http://%s.local", 
        hostname
        );
    
    // WAP or WAP SETUP, Default password or Custom. Sends to 
    // setWAPtype for modification.
    this->setWAPtype(WAPtype);
    strcpy(details.WAPtype, WAPtype);

    // REMAINING HEAP SIZE
    sprintf(details.heap, "%.2f KB", this->getHeapSize(HEAP_SIZE::KB));

    // CLIENTS CONNECTED
    wifi_sta_list_t sta_list;
    esp_wifi_ap_get_sta_list(&sta_list);
    sprintf(details.clientConnected, "%d", sta_list.num);
}

// Sets the mode (WAP or WAP SETUP), and if the APpass equals the 
// default password, it will add a suffix if (DEF) to display that 
// the WAP mode is in default settings.
void NetWAP::setWAPtype(char* WAPtype) {
    bool isDefault = (strcmp(this->APpass, this->APdefaultPass) == 0);
    const char* mode{NetMain::getNetType() == NetMode::WAP ? "WAP" : 
        "WAP SETUP"};
    const char* suffix{isDefault ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

}