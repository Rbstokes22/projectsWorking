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
    if (strlen(this->APpass) < 9) {
        this->sendErr("Open Network due to pass lgth < 9");
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

            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s: DHCPS not stopped: %s", this->tag, esp_err_to_name(stop));
            
            this->sendErr(NetMain::log);
            return wifi_ret_t::DHCPS_FAIL;
            }

        // Setup the ip, gateway, and netmask. This allows (255 - 2) Addresses.
        IP4_ADDR(&this->ip_info.ip, 192,168,1,1); 
        IP4_ADDR(&this->ip_info.gw, 192,168,1,1);
        IP4_ADDR(&this->ip_info.netmask, 255, 255, 255, 0);
        isInit = true; // Sets to true upon success and continutes.
    }
    
    // Checks to see that both the dhcp is off and ip is unset. If true
    // sets the ip info.
    if (!NetMain::flags.getFlag(NETFLAGS::dhcpOn) && 
        !NetMain::flags.getFlag(NETFLAGS::dhcpIPset)) { 

        // Attempts to set IP while dhcp is off.
        esp_err_t set_ip = esp_netif_set_ip_info(NetMain::ap_netif, 
            &this->ip_info);

        if (set_ip == ESP_OK) { // Indicates IP set
            NetMain::flags.setFlag(NETFLAGS::dhcpIPset);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s IP information not set. %s", this->tag, 
                esp_err_to_name(set_ip));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    // Checks that the dhcp is off and the IP is set. If true, starts the 
    // dhcp server.
    if (!NetMain::flags.getFlag(NETFLAGS::dhcpOn) &&
        NetMain::flags.getFlag(dhcpIPset)) { // dhcp not on, but IP is set.

        esp_err_t start = esp_netif_dhcps_start(NetMain::ap_netif);

        if (start == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED || 
            start == ESP_OK) {

            NetMain::flags.setFlag(NETFLAGS::dhcpOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s DHCPS not started. %s", this->tag, 
                esp_err_to_name(start));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    return wifi_ret_t::DHCPS_OK;
}

// Requires the ssid, default password, and mdns name.
NetWAP::NetWAP(const char* APssid, const char* APdefPass, 
    const char* mdnsName) : NetMain(mdnsName), tag("NETWAP") {

    strncpy(this->APssid, APssid, sizeof(this->APssid) - 1);
    this->APssid[sizeof(this->APssid) - 1] = '\0';

    strncpy(this->APpass, APdefPass, sizeof(this->APpass) - 1);
    this->APpass[sizeof(this->APpass) -1] = '\0';

    strcpy(this->APdefaultPass, APdefPass); // Always the same, no "n" req.

    snprintf(NetMain::log, sizeof(NetMain::log), "%s Ob created", this->tag);
    NetMain::sendErr(NetMain::log, Messaging::Levels::INFO);


}

// Second step in the init process. Steps 1 and 4 are in the NetMain src file.
// Requires no params. Runs the dhcps handler to set the IP, configures the
// wifi connection, sets the wifi mode and configuration, and starts the wifi.
// Returns WIFI_FAIL or WIFI_OK.
wifi_ret_t NetWAP::start_wifi() {

    // Always run, flags are withing the handler to ensure safety. Sets up
    // the IP address for the WAP connection.
    if (this->dhcpsHandler() == wifi_ret_t::DHCPS_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    // Always run, no flags required.
    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    // Sets the wifi mode to AP or Wireless mode.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiModeSet)) {
        
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_AP);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiModeSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s Wifi mode not set. %s", this->tag, 
                esp_err_to_name(wifi_mode));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Handles configuration of the wifi settings.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiConfigSet)) {
        
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_AP, 
                &this->wifi_config);

        if (wifi_cfg == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiConfigSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s Wifi config not set. %s", this->tag, 
                esp_err_to_name(wifi_cfg));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Handles the start of the wifi.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_start = esp_wifi_start();

        if (wifi_start == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s Wifi not started. %s", this->tag, 
                esp_err_to_name(wifi_start));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

// Third step of the init process. 4th step is back to the NetMain src file.
// Once the wifi connection is established, starts the httpd server and 
// registers the Universal Resource Identifiers (URI). Returns SERVER_FAIL or
// SERVER_OK.
wifi_ret_t NetWAP::start_server() {

    // Handles starting the http daemon or httpd server.
    if (!NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
        esp_err_t httpd = httpd_start(&NetMain::server, &NetMain::http_config);

        if (httpd == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::httpdOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s HTTP not started. %s", this->tag, 
                esp_err_to_name(httpd));

            this->sendErr(NetMain::log);
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    // Handles the registration of the uri handlers. The references passed in
    // the uri handlers are located on Routes header and source.
    if (!NetMain::flags.getFlag(NETFLAGS::uriReg)) {
        
        // This will register different URI's based on the type of WAP
        // that is set. This allows each version to use their own index
        // page, and avoids the need for complex URL's.
        if (NetMain::NetType == NetMode::WAP) {
            
            esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, 
                &WAPIndex);

            esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, &ws);

            if (reg1 == ESP_OK && reg2 == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::uriReg);
                return wifi_ret_t::SERVER_OK;

            } else {
                snprintf(NetMain::log, sizeof(NetMain::log),
                    "%s WAP URI registration fail", this->tag) ;
                  
                this->sendErr(NetMain::log);
                return wifi_ret_t::SERVER_FAIL;
            }

        } else if (NetMain::NetType == NetMode::WAP_SETUP) {
            esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, 
                &WAPSetupIndex);

            esp_err_t reg3 = httpd_register_uri_handler(NetMain::server, 
                &WAPSubmitCreds);

            if (reg2 == ESP_OK && reg3 == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::uriReg);

            } else {
                snprintf(NetMain::log, sizeof(NetMain::log),
                    "%s WAPSetup URI registration fail", this->tag) ;
                  
                this->sendErr(NetMain::log);
                return wifi_ret_t::SERVER_FAIL;
            }
        } 
    }

    return wifi_ret_t::SERVER_OK;
}
       
// Requires no params. Destroys WAP connection by stopping the mdns and httpd
// server, stopping wifi, disconnects WAP wifi, and stops the dhcp server. 
// Resets all required flags back to false, to allow re-init through the init
// sequences. Returns DESTROY_FAIL or DESTROY_OK.
wifi_ret_t NetWAP::destroy() { 

    // Destroys the mdns server.
    if (NetMain::flags.getFlag(NETFLAGS::mdnsInit)) {
        
        mdns_free();
        NetMain::flags.releaseFlag(mdnsInit);
        NetMain::flags.releaseFlag(mdnsHostSet);
        NetMain::flags.releaseFlag(mdnsInstanceSet);
        NetMain::flags.releaseFlag(mdnsServiceAdded);
    }

    // Destroys and stops the httpd connection
     if (NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
        esp_err_t httpd = httpd_stop(NetMain::server);

        if (httpd == ESP_OK) {
            NetMain::flags.releaseFlag(httpdOn);
            NetMain::flags.releaseFlag(uriReg);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s HTTPD not stopped. %s", this->tag, esp_err_to_name(httpd));
                  
            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    // Destroys and stops the wifi connection.
    if (NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_stop = esp_wifi_stop();

        if (wifi_stop == ESP_OK) {
            NetMain::flags.releaseFlag(wifiOn);
            NetMain::flags.releaseFlag(wifiModeSet);
            NetMain::flags.releaseFlag(dhcpIPset);
            NetMain::flags.releaseFlag(wifiConfigSet);
            
        } else {
            snprintf(NetMain::log, sizeof(NetMain::log),
                "%s Wifi not stopped. %s", this->tag, 
                esp_err_to_name(wifi_stop));
                  
            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    // Destroys and stops the dhcp server to allow reset of IP.
    if (NetMain::flags.getFlag(NETFLAGS::dhcpOn)) { 
        
        esp_err_t stop = esp_netif_dhcps_stop(NetMain::ap_netif);
        
        if (stop == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED || 
            stop == ESP_OK) {

            NetMain::flags.releaseFlag(dhcpOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s: DHCPS not stopped: %s", this->tag, esp_err_to_name(stop));
            
            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    return wifi_ret_t::DESTROY_OK;
}

// Requires the password text string. Sets the password. Max length 64 chars.
void NetWAP::setPass(const char* pass) {

    // Checks that pass is actually passed. Copies pass to object variable.
    if (strlen(pass) != 0 && pass != nullptr) {
        strncpy(this->APpass, pass, sizeof(this->APpass) - 1);
        this->APpass[sizeof(this->APpass) - 1] = '\0'; // ensure null term.
    } else {
        snprintf(NetMain::log, sizeof(NetMain::log), "%s Pass not set", 
            this->tag);

        this->sendErr(NetMain::log);
    }
}

// Requires bool if defaultPassword return. If true, returns the default 
// WAP password, if false, returns the set password. Used for initial startup
// and following startups until password is saved into NVS, or the default
// pass button is not pressed indicating default mode.
const char* NetWAP::getPass(bool defaultPass) const {
    if (!defaultPass) {
        return this->APpass;
    } else {
        return this->APdefaultPass;
    }
}

// Abstract methods not needed in the scope of NetWAP. Required by blueprint.
void NetWAP::setSSID(const char* ssid) {}
const char* NetWAP::getSSID() const {return "";}

// Runs an iteration of all flags pertaining to the station 
// connection. Upon all flags being set and the station 
// being connected, returns true. Returns false for failure.
bool NetWAP::isActive() { 
    wifi_mode_t mode;
    const uint8_t flagRequirement = 15; // Number of flags to check.
    uint8_t flagSuccess = 0;

    // All the required flags to verify active broadcast.
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

    // Iterate each flag.
    for (int i = 0; i < flagRequirement; i++) {
        flagSuccess += required[i]; // Increments if true.
    }

    if ((flagSuccess == flagRequirement) && 
        esp_wifi_get_mode(&mode) == ESP_OK &&
        mode == WIFI_MODE_AP) { // Means probably connected

        wifi_config_t ap_config;
        if (esp_wifi_get_config(WIFI_IF_AP, &ap_config) == ESP_OK) {  // Conn
    
            if (this->getLogToggle()->con) {
                snprintf(NetMain::log, sizeof(NetMain::log), 
                    "%s Connected", this->tag);

                this->sendErr(NetMain::log, Messaging::Levels::INFO);
                this->getLogToggle()->con = false; // Block another attempt
                this->getLogToggle()->discon = true; // Reset if prev false
            }

            return true; // Actively connected. Block.
        }
    } 

    // Run this block if not connected. 
    if (this->getLogToggle()->discon) {
        snprintf(NetMain::log, sizeof(NetMain::log), 
            "%s Disconnected", this->tag);

        this->sendErr(NetMain::log, Messaging::Levels::INFO);
        this->getLogToggle()->discon = false; // Block another attempt
        this->getLogToggle()->con = true; // Reset if previously false.
    }

    return false;
}

// Requires WAP details reference. Builds the passed detail struct containing
// the ip address, broadcast status, mdns name, type of WAP broadcast, free
// heap memory, and clients connected.
void NetWAP::getDetails(WAPdetails &details) {

    char status[2][4] = {"No", "Yes"}; // used to display broadcast status
    char WAPtype[20]{0};

    // IP. Copies the default IP Address to the details struct.
    snprintf(details.ipaddr, sizeof(details.ipaddr), "http://192.168.1.1");

    // Status. displays "Yes" or "No" depending on broadcast activity.
    strcpy(details.status, status[this->isActive()]);

    // MDNS. Copies mDNS over to the details struct.
    char hostname[static_cast<int>(IDXSIZE::MDNS)];
    mdns_hostname_get(hostname); // Gets hostname.
    snprintf(details.mdns, sizeof(details.mdns), "http://%s.local", hostname);
    
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

// Requires the WAPtype char pointer. Populates the WAPtype array with the
// correct text indicating the type and if default password is set.
void NetWAP::setWAPtype(char* WAPtype) {

    // Checks the current set password against the default password. If equal,
    // bool is set to true.
    bool isDefault = (strcmp(this->APpass, this->APdefaultPass) == 0);

    // Next, sets the mode pointer to either WAP or WAP SETUP depending on
    // toggle position.
    const char* mode{NetMain::getNetType() == NetMode::WAP ? "WAP" : 
        "WAP SETUP"};

    // if isDefault is true, will append the type with (DEF). Ignores otherwise.
    const char* suffix{isDefault ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

}