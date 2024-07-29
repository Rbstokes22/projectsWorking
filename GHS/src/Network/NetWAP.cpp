#include "Network/NetWAP.hpp"
#include "Network/Routes.hpp"
#include "string.h"
#include "lwip/inet.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <cstddef>

// ADD LOGIC TO DIFFERENTIATE BETWEEN WAP AND WAP_SETUP USE CREDS IN NET MANAGER
// TO PASS DATA HERE

namespace Comms {

// PRIVATE
wifi_ret_t NetWAP::configure() {
    uint8_t maxConnections{4};

    // Extra layer of security since this is on http.
    if (NetMain::NetType == NetMode::WAP_SETUP) maxConnections = 1;

    strcpy((char*)this->wifi_config.ap.ssid, this->APssid);
    this->wifi_config.ap.ssid_len = strlen(this->APssid);
    this->wifi_config.ap.channel = 6;
    strcpy((char*)this->wifi_config.ap.password, this->APpass);
    this->wifi_config.ap.max_connection = maxConnections;
    this->wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    

    if (strlen(this->APpass) < 8) {
        this->sendErr("Open Network due to pass len");
        this->wifi_config.ap.authmode = WIFI_AUTH_OPEN; // No password, open network
    }

    return wifi_ret_t::CONFIG_OK;
}

wifi_ret_t NetWAP::dhcpsHandler() {
    esp_err_t dhcps_stop = esp_netif_dhcps_stop(this->ap_netif); // stop if already running

    if (dhcps_stop != ESP_OK) {
        this->sendErr("DHCPS not stopped");
        return wifi_ret_t::DHCPS_FAIL;
    }

    IP4_ADDR(&this->ip_info.ip, 192,168,1,1); 
    IP4_ADDR(&this->ip_info.gw, 192,168,1,1);
    IP4_ADDR(&this->ip_info.netmask, 255, 255, 255, 0);

    esp_err_t set_ip = esp_netif_set_ip_info(this->ap_netif, &this->ip_info);
    esp_err_t dhcps_start = esp_netif_dhcps_start(this->ap_netif);

    uint8_t errCt{0};
    esp_err_t errors[]{set_ip, dhcps_start};
    const char errMsg[2][25] = {
        "IP unable to start", 
        "DHCPS unable to start"
        };
    
    for (int i = 0; i < 2; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            errCt++;
        }
    }

    if (errCt > 0) {return wifi_ret_t::DHCPS_FAIL;}
    else {return wifi_ret_t::DHCPS_OK;}
}

// PUBLIC
NetWAP::NetWAP(
    Messaging::MsgLogHandler &msglogerr,
    const char* APssid, const char* APdefPass) : 

    NetMain(msglogerr), isInit(false), ap_netif(nullptr),
    wifi_config({}) {

        strncpy(this->APssid, APssid, sizeof(this->APssid) - 1);
        this->APssid[sizeof(this->APssid) - 1] = '\0';

        strncpy(this->APpass, APdefPass, sizeof(this->APpass) - 1);
        this->APpass[sizeof(this->APpass) -1] = '\0';
    }

wifi_ret_t NetWAP::init_wifi() {

    esp_err_t init = esp_netif_init();
    esp_err_t event = esp_event_loop_create_default();
    this->ap_netif = esp_netif_create_default_wifi_ap();
    this->server_config = HTTPD_DEFAULT_CONFIG();

    if (init == ESP_OK && event == ESP_OK && ap_netif != nullptr) {
        return wifi_ret_t::INIT_OK;
    } else {
        this->sendErr("AP unable to init");
        return wifi_ret_t::INIT_FAIL;
    }
}

wifi_ret_t NetWAP::start_wifi() {

    if (this->dhcpsHandler() == wifi_ret_t::DHCPS_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL;
    }

    esp_err_t set_mode = esp_wifi_set_mode(WIFI_MODE_AP);
    esp_err_t set_config = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_err_t start_wifi = esp_wifi_start();

    // Error handling
    uint8_t errCt{0};
    esp_err_t errors[]{set_mode, set_config, start_wifi};
    const char errMsg[3][25] = {
        "Wifi mode not set",
        "Wifi config not set",
        "Wifi unable to start"
        };
    
    for (int i = 0; i < 3; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            errCt++;
        }
    }

    if (errCt > 0) {return wifi_ret_t::WIFI_FAIL;}
    else {return wifi_ret_t::WIFI_OK;}
}

wifi_ret_t NetWAP::start_server() {
 
    if (httpd_start(&this->server, &this->server_config) != ESP_OK) {
        return wifi_ret_t::SERVER_FAIL;
    }

    if (NetMain::NetType == NetMode::WAP) {
        esp_err_t reg1 = httpd_register_uri_handler(this->server, &WAPIndex);

        if (reg1 == ESP_OK) {
            return wifi_ret_t::SERVER_OK;
        } else {
            this->sendErr("Unable to register uri");
            return wifi_ret_t::SERVER_FAIL;
        }

    } else if (NetMain::NetType == NetMode::WAP_SETUP) {
        esp_err_t reg1 = httpd_register_uri_handler(this->server, &WAPSetupIndex);
        esp_err_t reg2 = httpd_register_uri_handler(this->server, &WAPSubmitCreds);

        if (reg1 == ESP_OK && reg2 == ESP_OK) {
            return wifi_ret_t::SERVER_OK;
        } else {
            this->sendErr("Unable to register uri");
            return wifi_ret_t::SERVER_FAIL;
        }

    } else {
        this->sendErr("Unable to start server");
        return wifi_ret_t::SERVER_FAIL;
    }
}
        
wifi_ret_t NetWAP::destroy() {

    esp_err_t server = httpd_stop(this->server);
    esp_err_t wifi = esp_wifi_stop();
    
    uint8_t errCt{0};
    esp_err_t errors[2] = {server, wifi};
    const char errMsg[2][25] = {
        "Server not destroyed",
        "Wifi not destroyed"
    };

    for (int i = 0; i < 2; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            errCt++;
        }
    }

    if (errCt > 0) {return wifi_ret_t::DESTROY_FAIL;}
    else {return wifi_ret_t::DESTROY_OK;}
}

// Only sets a new password if one exists.
void NetWAP::setPass(const char* pass) {
    if (strlen(pass) != 0) {
        strncpy(this->APpass, pass, sizeof(this->APpass) -1);
        this->APpass[sizeof(this->APpass) - 1] = '\0';
    } 
    printf("pass: %s\n", pass); // DELETE AFTER TESTING
}

// Abstract methods not needed in the scope of NetWAP.
void NetWAP::setSSID(const char* ssid) {}
void NetWAP::setPhone(const char* phone) {}

bool NetWAP::isInitialized() {
    return this->isInit;
}

void NetWAP::setInit(bool newInit) {
    this->isInit = newInit;
}

void NetWAP::getDetails(WAPdetails &details) {

    char status[2][4] = {"No", "Yes"}; // used to display connection
    char WAPtype[20]{0};

    // IPADDR & CONNECTION STATUS
    strcpy(details.ipaddr, "192.168.1.1");
    strcpy(details.status, status[this->isBroadcasting()]);
    
    // WAP or WAP SETUP, Default password or Custom
    this->setWAPtype(WAPtype);
    strcpy(details.WAPtype, WAPtype);

    // REMAINING HEAP SIZE
    sprintf(details.heap, "%.2f%%", this->getHeapSize());

    // CLIENTS CONNECTED
    wifi_sta_list_t sta_list;
    esp_wifi_ap_get_sta_list(&sta_list);
    sprintf(details.clientConnected, "%d", sta_list.num);
}

// Checks that the mode and config are set to AP and there are no issues.
bool NetWAP::isBroadcasting() {
    wifi_mode_t mode;

    if (esp_wifi_get_mode(&mode) == ESP_OK && mode == WIFI_MODE_AP) {
        wifi_config_t ap_config;
        if (esp_wifi_get_config(WIFI_IF_AP, &ap_config) == ESP_OK) {
            return true;
        }
    }

    return false;
}

void NetWAP::setWAPtype(char* WAPtype) {
    bool isDefault = (strcmp(this->APpass, "12345678") == 0);
    const char* mode{this->getNetType() == NetMode::WAP ? "WAP" : "WAP SETUP"};
    const char* suffix{isDefault ? " (DEF)" : ""};
    sprintf(WAPtype, "%s%s", mode, suffix);
}

}