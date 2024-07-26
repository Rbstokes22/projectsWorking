#include "Network/NetWAP.hpp"
#include "Network/Routes.hpp"

// ADD LOGIC TO DIFFERENTIATE BETWEEN WAP AND WAP_SETUP USE CREDS IN NET MANAGER
// TO PASS DATA HERE

namespace Comms {

NetWAP::NetWAP(
    Messaging::MsgLogHandler &msglogerr,
    const char* APssid, const char* APdefPass) : 

    NetMain(msglogerr) {
        strncpy(this->APssid, APssid, sizeof(this->APssid) - 1);
        this->APssid[sizeof(this->APssid) - 1] = '\0';

        strncpy(this->APpass, APdefPass, sizeof(this->APpass) - 1);
        this->APpass[sizeof(this->APpass) -1] = '\0';
    }

wifi_ret_t NetWAP::init_wifi() {

    esp_err_t init = esp_netif_init();
    esp_err_t event = esp_event_loop_create_default();

    if (init != ESP_OK && event != ESP_OK) {
        this->sendErr("AP unable to init");
        return wifi_ret_t::INIT_FAIL;
    } else {
        return wifi_ret_t::INIT_OK;
    }
}

wifi_ret_t NetWAP::start_wifi() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    esp_err_t dhcps_stop = esp_netif_dhcps_stop(ap_netif); // stop if already running
    if (dhcps_stop != ESP_OK) {
        this->sendErr("DHCPS not stopped");
        return wifi_ret_t::WIFI_FAIL;
    }

    IP4_ADDR(&ip_info.ip, 192,168,1,1); 
    IP4_ADDR(&ip_info.gw, 192,168,1,1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    wifi_config_t wifi_config = {};

    // Allows a single connection during WAP_SETUP since it uses http, 
    // and prevents multiple connections to increase security.
    if (NetMain::NetType == NetMode::WAP_SETUP) {
        configure(wifi_config, 1);
    } else {
        configure(wifi_config, 4); // regular max con 4
    }

    esp_err_t set_ip = esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_err_t dhcps_start = esp_netif_dhcps_start(ap_netif);
    esp_err_t set_mode = esp_wifi_set_mode(WIFI_MODE_AP);
    esp_err_t set_config = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_err_t start_wifi = esp_wifi_start();

    // Error handling
    esp_err_t errors[]{set_ip, dhcps_start, set_mode, set_config, start_wifi};
    const char errMsg[5][25] = {
        "IP unable to start", 
        "DHCPS unable to start",
        "Wifi mode not set",
        "Wifi config not set",
        "Wifi unable to start"
        };
    
    for (int i = 0; i < 5; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

wifi_ret_t NetWAP::start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    return wifi_ret_t::SERVER_FAIL;

    if (httpd_start(&this->server, &config) != ESP_OK) {
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
    if (httpd_stop(this->server) != ESP_OK) {
        this->sendErr("Server not destroyed");
        return wifi_ret_t::DESTROY_FAIL;
    } else {
        return wifi_ret_t::DESTROY_OK;
    }
}

// Only sets a new password if one exists.
void NetWAP::setPass(const char* pass) {
    if (strlen(pass) != 0) {
        strncpy(this->APpass, pass, sizeof(this->APpass) -1);
        this->APpass[sizeof(this->APpass) - 1] = '\0';
    } 
}

// Abstract methods not needed in the scope of NetWAP.
void NetWAP::setSSID(const char* ssid) {}
void NetWAP::setPhone(const char* phone) {}

void NetWAP::configure(wifi_config_t &wifi_config, uint8_t maxConnections) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t wifi_init = esp_wifi_init(&cfg);

    if (wifi_init != ESP_OK) {
        this->sendErr("AP not configured");
        return;
    }

    strcpy((char*)wifi_config.ap.ssid, this->APssid);
    wifi_config.ap.ssid_len = strlen(this->APssid);
    wifi_config.ap.channel = 6;
    strcpy((char*)wifi_config.ap.password, this->APpass);
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    

    if (strlen(this->APpass) < 8) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN; // No password, open network
    }
}

}