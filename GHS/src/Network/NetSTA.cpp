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
wifi_ret_t NetSTA::configure() { // NOT CONNECTING
    strcpy((char*)this->wifi_config.sta.ssid, "Bulbasaur");
    strcpy((char*)this->wifi_config.sta.password, "Castiel1"); 

    return wifi_ret_t::CONFIG_OK;
}

void NetSTA::IPEvent(
    void* arg, 
    esp_event_base_t eventBase, 
    int32_t eventID, 
    void* eventData) {

    if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
        esp_netif_ip_info_t ip_info = event->ip_info;

        esp_ip4addr_ntoa(&ip_info.ip, NetSTA::IPADDR, sizeof(NetSTA::IPADDR));
        printf("Got IP: %s\n", NetSTA::IPADDR);
    }
}

// PUBLIC
NetSTA::NetSTA(Messaging::MsgLogHandler &msglogerr) : 

    NetMain(msglogerr) {

        memset(this->ssid, 0, sizeof(this->ssid));
        memset(this->pass, 0, sizeof(this->pass));
        memset(this->phone, 0, sizeof(this->phone));
    }

wifi_ret_t NetSTA::start_wifi() {

    if (!NetMain::flags.handlerReg) {
        esp_err_t handler = esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent, NULL);

        if (handler == ESP_OK) {
            NetMain::flags.handlerReg = true;
        } else {
            this->sendErr("Handler not registered", errDisp::ALL);
            this->sendErr(esp_err_to_name(handler), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Always run, no flag.
    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL; // error prints in configure
    } 

    if (!NetMain::flags.wifiModeSet) {
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_STA);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.wifiModeSet = true;
        } else {
            this->sendErr("Wifi mode not set", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_mode), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    if (!NetMain::flags.wifiConfigSet) {
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_STA, &this->wifi_config);

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

    if (!NetMain::flags.wifiConnected) {
        esp_err_t wifi_con = esp_wifi_connect();

        if (wifi_con == ESP_OK) {
            NetMain::flags.wifiConnected = true;
        } else {
            this->sendErr("Wifi not connected", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_con), errDisp::SRL);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

wifi_ret_t NetSTA::start_server() {

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
        esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, &STAIndex);

        if (reg1 == ESP_OK) {
                NetMain::flags.uriReg = true;
        } else {
            this->sendErr("STA URI's unregistered", errDisp::ALL);
            this->sendErr(esp_err_to_name(reg1), errDisp::SRL);
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    return wifi_ret_t::SERVER_OK;
}

wifi_ret_t NetSTA::destroy() {

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
            NetMain::flags.wifiConfigSet = false;
        } else {
            this->sendErr("Wifi not stopped", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_stop), errDisp::SRL);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.wifiConnected) {
        esp_err_t wifi_con = esp_wifi_disconnect();

        if (wifi_con == ESP_OK || wifi_con == ESP_ERR_WIFI_NOT_STARTED) {
            NetMain::flags.wifiConnected = false;
        } else {
            this->sendErr("Wifi not disconnected", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_con), errDisp::SRL);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    if (NetMain::flags.handlerReg) {
        esp_err_t event = esp_event_handler_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent);

        if (event == ESP_OK) {
            NetMain::flags.handlerReg = false;
        } else {
            this->sendErr("Wifi handler not unregistered", errDisp::ALL);
            this->sendErr(esp_err_to_name(event), errDisp::SRL);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    return wifi_ret_t::DESTROY_OK;
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

bool NetSTA::isActive() {
    wifi_ap_record_t sta_info;

    uint8_t flagRequirement = 11;
    uint8_t flagSuccess = 0;

    bool required[flagRequirement]{
        NetMain::flags.wifiInit, NetMain::flags.netifInit,
        NetMain::flags.eventLoopInit, NetMain::flags.sta_netifCreated,
        NetMain::flags.wifiModeSet, NetMain::flags.wifiConfigSet,
        NetMain::flags.wifiOn, NetMain::flags.httpdOn,
        NetMain::flags.uriReg, NetMain::flags.handlerReg,
        NetMain::flags.wifiConnected
    };

    for (int i = 0; i < flagRequirement; i++) {
        if (required[i] == true) flagSuccess++;
    }

    if (flagSuccess != flagRequirement) {
        this->sendErr("Wifi flags not met", errDisp::SRL);
        return false;
    } else if (esp_wifi_sta_get_ap_info(&sta_info) == ESP_OK) {
        return true;
    } 

    return false;
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
    strcpy(details.status, status[this->isActive()]);
    
    // RSSI signal strength 
    sprintf(details.signalStrength, "%d dBm", sta_info.rssi);

    // REMAINING HEAP SIZE
    sprintf(details.heap, "%.2f KB", this->getHeapSize(HEAP_SIZE::KB));
}

}