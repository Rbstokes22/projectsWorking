#include "Network/NetSTA.hpp"
#include "Network/Routes.hpp"
#include "string.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"

namespace Comms {

NetSTA::NetSTA(Messaging::MsgLogHandler &msglogerr) : 
    NetMain(msglogerr) {

        memset(this->ssid, 0, sizeof(this->ssid));
        memset(this->pass, 0, sizeof(this->pass));
        memset(this->phone, 0, sizeof(this->phone));
    }

wifi_ret_t NetSTA::init_wifi() {
    esp_err_t init = esp_netif_init();
    esp_err_t event = esp_event_loop_create_default();

    if (init == ESP_OK && event == ESP_OK) {
        return wifi_ret_t::INIT_OK;
    } else {
        this->sendErr("AP unable to init");
        return wifi_ret_t::INIT_FAIL;
    }
}

// COPY EVERYTHING FROM THE WAP HERE AND DIFFERENTIATE 
wifi_ret_t NetSTA::start_wifi() {
    esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = {};
    if (this->configure(wifi_config) != wifi_ret_t::CONFIG_OK) {
        return wifi_ret_t::WIFI_FAIL; // error print in configure
    }

    esp_err_t set_mode = esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t set_config = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_err_t start_wifi = esp_wifi_start();

    esp_err_t errors[]{set_mode, set_config, start_wifi};
    const char errMsg[3][25] = {
        "Wifi mode not set",
        "Wifi config not set",
        "Wifi unable to start"
        };
    
    for (int i = 0; i < 3; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            return wifi_ret_t::WIFI_FAIL;
        }
    }
    
    return wifi_ret_t::WIFI_OK;
}

wifi_ret_t NetSTA::start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&this->server, &config) != ESP_OK) {
        return wifi_ret_t::SERVER_FAIL;
    }

    esp_err_t reg1 = httpd_register_uri_handler(this->server, &STAIndex);

    if (reg1 == ESP_OK) {
        return wifi_ret_t::SERVER_OK;
    } else {
        this->sendErr("Unable to register uri");
        return wifi_ret_t::SERVER_FAIL;
    }
}

wifi_ret_t NetSTA::destroy() {
    if (httpd_stop(this->server) != ESP_OK) {
        this->sendErr("Server not destroyed");
        return wifi_ret_t::DESTROY_FAIL;
    } else {
        return wifi_ret_t::DESTROY_OK;
    }
}

void NetSTA::setPass(const char* pass) {
    if (strlen(pass) != 0) {
        strncpy(this->pass, pass, sizeof(this->pass) -1);
        this->pass[sizeof(this->pass) - 1] = '\0';
    } 
    printf("pass: %s\n", pass); // DELETE AFTER TESTING
}

void NetSTA::setSSID(const char* ssid) {
    if (strlen(ssid) != 0) {
        strncpy(this->ssid, ssid, sizeof(this->ssid) -1);
        this->ssid[sizeof(this->ssid) - 1] = '\0';
    } 
    printf("pass: %s\n", ssid); // DELETE AFTER TESTING
}

void NetSTA::setPhone(const char* phone) {
    if (strlen(phone) != 0) {
        strncpy(this->phone, phone, sizeof(this->phone) -1);
        this->phone[sizeof(this->phone) - 1] = '\0';
    } 
    printf("pass: %s\n", phone); // DELETE AFTER TESTING
}

wifi_ret_t NetSTA::configure(wifi_config_t &wifi_config) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t wifi_init = esp_wifi_init(&cfg);

    if (wifi_init != ESP_OK) {
        this->sendErr("STA not configured");
        return wifi_ret_t::CONFIG_FAIL;
    }

    strcpy((char*)wifi_config.sta.ssid, this->ssid);
    strcpy((char*)wifi_config.sta.password, this->pass);
    return wifi_ret_t::CONFIG_OK;
}

}