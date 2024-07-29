#include "Network/NetSTA.hpp"
#include "Network/Routes.hpp"
#include "string.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <cstddef>

namespace Comms {

// STATIC
char NetSTA::IPADDR[static_cast<int>(IDXSIZE::IPADDR)]{0};

// PRIVATE
wifi_ret_t NetSTA::configure(wifi_config_t &wifi_config) {

    strcpy((char*)wifi_config.sta.ssid, "Bulbasaur"); // replace with ssid
    strcpy((char*)wifi_config.sta.password, "Castiel1"); // replace with pass
    return wifi_ret_t::CONFIG_OK;
}

void NetSTA::IPEvent(
    void* arg, 
    esp_event_base_t eventBase, 
    int32_t eventID, 
    void* eventData) {
    
    ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
    esp_netif_ip_info_t ip_info = event->ip_info;

    esp_ip4addr_ntoa(&ip_info.ip, NetSTA::IPADDR, sizeof(NetSTA::IPADDR));
    }

// PUBLIC
NetSTA::NetSTA(Messaging::MsgLogHandler &msglogerr) : 
    NetMain(msglogerr), isInit(false) {

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

wifi_ret_t NetSTA::start_wifi() {
    esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_config = {};
    if (this->configure(wifi_config) != wifi_ret_t::CONFIG_OK) {
        return wifi_ret_t::WIFI_FAIL; // error prints in configure
    } 

    esp_err_t set_mode = esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t set_config = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_err_t start_wifi = esp_wifi_start();

    // register the IP event to set the assigned ip addr.
    esp_err_t registerIPEvent = esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent, NULL
    );

    uint8_t errCt{0};
    esp_err_t errors[]{set_mode, set_config, start_wifi, registerIPEvent};
    const char errMsg[4][28] = {
        "Wifi mode not set",
        "Wifi config not set",
        "Wifi unable to start",
        "IP event not registered"
        };
    
    for (int i = 0; i < 4; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            errCt++;
        }
    }
    
    if (errCt > 0) {return wifi_ret_t::WIFI_FAIL;}
    else {return wifi_ret_t::WIFI_OK;}
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

    esp_err_t server = httpd_stop(this->server);
    esp_err_t wifi = esp_wifi_stop();
    esp_err_t event = esp_event_handler_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent
    );

    uint8_t errCt{0};
    esp_err_t errors[3] = {server, wifi, event};
    const char errMsg[3][25] = {
        "Server not destroyed",
        "Wifi not destroyed",
        "IP event not destroyed"
    };

    for (int i = 0; i < 3; i++) {
        if (errors[i] != ESP_OK) {
            this->sendErr(errMsg[i]);
            errCt++;
        }
    }

    if (errCt > 0) {return wifi_ret_t::DESTROY_FAIL;}
    else {return wifi_ret_t::DESTROY_OK;}
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
    printf("ssid: %s\n", ssid); // DELETE AFTER TESTING
}

void NetSTA::setPhone(const char* phone) {
    if (strlen(phone) != 0) {
        strncpy(this->phone, phone, sizeof(this->phone) -1);
        this->phone[sizeof(this->phone) - 1] = '\0';
    } 
    printf("phone: %s\n", phone); // DELETE AFTER TESTING
}

bool NetSTA::isInitialized() {
    return this->isInit;
}

void NetSTA::setInit(bool newInit) {
    this->isInit = newInit;
}

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
    strcpy(details.ipaddr, NetSTA::IPADDR);

    // STATUS
    strcpy(details.status, status[this->isConnected()]);
    
    // RSSI signal strength 
    sprintf(details.signalStrength, "%d dBm", sta_info.rssi);

    // REMAINING HEAP SIZE
    sprintf(details.heap, "%.2f%%", this->getHeapSize());

}

bool NetSTA::isConnected() {
    wifi_ap_record_t sta_info;
    if (esp_wifi_sta_get_ap_info(&sta_info) == ESP_OK) {
        return true;
    } else {
        return false;
    }
}

}