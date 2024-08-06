#include "Network/NetWAP.hpp"
#include "Network/Routes.hpp"
#include "string.h"
#include "lwip/inet.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <cstddef>

namespace Comms {

// PRIVATE

// Sets the wifi configuration with the WAP ssid, password,
// channel, maximum connections, and authmode. Will use a 
// max connection default of 1 while in WAP_SETUP mode as 
// an extra layer of security since communications are on
// http, to prevent malicious connections from intercepting
// data. Returns CONFIG_OK.
wifi_ret_t NetWAP::configure() {
    uint8_t maxConnections{4};

    if (NetMain::NetType == NetMode::WAP_SETUP) maxConnections = 1;

    strcpy((char*)this->wifi_config.ap.ssid, this->APssid);
    this->wifi_config.ap.ssid_len = strlen(this->APssid);
    this->wifi_config.ap.channel = 6;
    strcpy((char*)this->wifi_config.ap.password, this->APpass);
    this->wifi_config.ap.max_connection = maxConnections;
    this->wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    
    if (strlen(this->APpass) < 8) {
        this->sendErr("Open Network due to pass lgth", errDisp::ALL);
        this->wifi_config.ap.authmode = WIFI_AUTH_OPEN; // No password, open network
    }

    return wifi_ret_t::CONFIG_OK;
}

// Dynamic Host Configuration Protocol (DHCP) will stop to allow 
// IP information to be configured to the device for WAP 
// connection. The IP information is set, which will allow the 
// DHCP to restart. Returns DHCPS_FAIL or DHCPS_OK.
wifi_ret_t NetWAP::dhcpsHandler() {
    
    if (NetMain::flags.dhcpOn) {
        esp_err_t stop = esp_netif_dhcps_stop(NetMain::ap_netif);

        if (stop == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED || 
            stop == ESP_OK) {

            NetMain::flags.dhcpOn = false;

        } else {
            this->sendErr("DHCPS not stopped", errDisp::ALL);
            this->sendErr(esp_err_to_name(stop), errDisp::SRL);
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    // Stays constant.
    IP4_ADDR(&this->ip_info.ip, 192,168,1,1); 
    IP4_ADDR(&this->ip_info.gw, 192,168,1,1);
    IP4_ADDR(&this->ip_info.netmask, 255, 255, 255, 0);

    if (!NetMain::flags.dhcpOn) {

        if (!NetMain::flags.dhcpIPset) {
            esp_err_t set_ip = esp_netif_set_ip_info(
                NetMain::ap_netif, 
                &this->ip_info);

            // This will force a stop if this error is returned, 
            // allowing a proper initialization.
            if (set_ip == ESP_ERR_ESP_NETIF_DHCP_NOT_STOPPED) {
                NetMain::flags.dhcpOn = true;
            }

            if (set_ip == ESP_OK) {
                    NetMain::flags.dhcpIPset = true;
            } else {
                this->sendErr("IP info not set", errDisp::ALL);
                this->sendErr(esp_err_to_name(set_ip), errDisp::SRL);
                return wifi_ret_t::DHCPS_FAIL;
            }
        }

        esp_err_t start = esp_netif_dhcps_start(NetMain::ap_netif);

        if (start == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED || 
            start == ESP_OK) {

            NetMain::flags.dhcpOn = true;

        } else {
            this->sendErr("DHCPS not started", errDisp::ALL);
            this->sendErr(esp_err_to_name(start), errDisp::SRL);
            return wifi_ret_t::DHCPS_FAIL;
        }
    }

    return wifi_ret_t::DHCPS_OK;
}

// PUBLIC

NetWAP::NetWAP(
    Messaging::MsgLogHandler &msglogerr,
    const char* APssid, const char* APdefPass) : 

    NetMain(msglogerr) {

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

    if (!NetMain::flags.wifiModeSet) {
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_AP);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.wifiModeSet = true;
        } else {
            this->sendErr("Wifi mode not set", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_mode), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.wifiConfigSet) {
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_AP, &this->wifi_config);

        if (wifi_cfg == ESP_OK) {
            NetMain::flags.wifiConfigSet = true;
        } else {
            this->sendErr("Wifi config not set", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_cfg), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.wifiOn) {
        esp_err_t wifi_start = esp_wifi_start();

        if (wifi_start == ESP_OK) {
            NetMain::flags.wifiOn = true;
        } else {
            this->sendErr("Wifi not started", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_start), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

// Third and last step in the init process. 
// Once the wifi connection is established, starts the httpd
// server and registers the Universal Resource Identifiers
// (URI). Returns SERVER_FAIL or SERVER_OK.
wifi_ret_t NetWAP::start_server() {

    if (!NetMain::flags.httpdOn) {
        esp_err_t httpd = httpd_start(&NetMain::server, &NetMain::server_config);

        if (httpd == ESP_OK) {
            NetMain::flags.httpdOn = true;
        } else {
            this->sendErr("HTTPD not started", errDisp::ALL);
            this->sendErr(esp_err_to_name(httpd), errDisp::SRL);
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    if (!NetMain::flags.uriReg) {

        // This will register different URI's based on the type of WAP
        // that is set. This allows each version to use their own index
        // page, and avoids the need for complex URL's.
        if (NetMain::NetType == NetMode::WAP) {
            
            esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, &WAPIndex);

            if (reg1 == ESP_OK) {
                NetMain::flags.uriReg = true;
                return wifi_ret_t::SERVER_OK;
            } else {
                this->sendErr("WAP URI's unregistered", errDisp::ALL);
                this->sendErr(esp_err_to_name(reg1), errDisp::SRL);
                return wifi_ret_t::SERVER_FAIL;
            }

        } else if (NetMain::NetType == NetMode::WAP_SETUP) {
            esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, &WAPSetupIndex);
            esp_err_t reg3 = httpd_register_uri_handler(NetMain::server, &WAPSubmitCreds);

            if (reg2 == ESP_OK && reg3 == ESP_OK) {
                NetMain::flags.uriReg = true;
            } else {
                this->sendErr("WAPSetup URI's unregistered", errDisp::ALL);
                this->sendErr(esp_err_to_name(reg2), errDisp::SRL);
                this->sendErr(esp_err_to_name(reg3), errDisp::SRL);
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

    if (NetMain::flags.httpdOn) {
        esp_err_t httpd = httpd_stop(NetMain::server);

        if (httpd == ESP_OK) {
            NetMain::flags.httpdOn = false;
            NetMain::flags.uriReg = false;
        } else {
            this->sendErr("HTTPD not stopped", errDisp::ALL);
            this->sendErr(esp_err_to_name(httpd), errDisp::SRL);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.wifiOn) {
        esp_err_t wifi_stop = esp_wifi_stop();

        if (wifi_stop == ESP_OK) {
            NetMain::flags.wifiOn = false;
            NetMain::flags.wifiModeSet = false;
            NetMain::flags.dhcpIPset = false;
            NetMain::flags.wifiConfigSet = false;
        } else {
            this->sendErr("Wifi not stopped", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_stop), errDisp::SRL);
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

const char* NetWAP::getPass(bool def) const {
    if (!def) {
        return this->APpass;
    } else {
        return this->APdefaultPass;
    }
}

// Abstract methods not needed in the scope of NetWAP.
void NetWAP::setSSID(const char* ssid) {}
void NetWAP::setPhone(const char* phone) {}
const char* NetWAP::getPhone() const {return "";}
const char* NetWAP::getSSID() const {return "";}

// Runs an iteration of all flags pertaining to the station 
// connection. Upon all flags being set and the station 
// being connected, returns true. Returns false for failure.
bool NetWAP::isActive() { 
    wifi_mode_t mode;
    uint8_t flagRequirement = 11;
    uint8_t flagSuccess = 0;

    bool required[flagRequirement]{
        NetMain::flags.wifiInit, NetMain::flags.netifInit,
        NetMain::flags.eventLoopInit, NetMain::flags.ap_netifCreated,
        NetMain::flags.dhcpOn, NetMain::flags.dhcpIPset,
        NetMain::flags.wifiModeSet, NetMain::flags.wifiConfigSet,
        NetMain::flags.wifiOn, NetMain::flags.httpdOn,
        NetMain::flags.uriReg
    };

    for (int i = 0; i < flagRequirement; i++) {
        if (required[i] == true) flagSuccess++;
    }

    if (flagSuccess != flagRequirement) {
        this->sendErr("Wifi flags not met", errDisp::SRL);
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
    strcpy(details.ipaddr, "192.168.1.1");
    strcpy(details.status, status[this->isActive()]);
    
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
    const char* mode{this->getNetType() == NetMode::WAP ? "WAP" : "WAP SETUP"};
    const char* suffix{isDefault ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

}