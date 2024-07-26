#include "Network/NetSTA.hpp"

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

    if (init != ESP_OK && event != ESP_OK) {
        this->sendErr("AP unable to init");
        return wifi_ret_t::INIT_FAIL;
    } else {
        return wifi_ret_t::INIT_OK;
    }
}

// COPY EVERYTHING FROM THE WAP HERE AND DIFFERENTIATE 
wifi_ret_t NetSTA::start_wifi() {
    return wifi_ret_t::WIFI_OK;
}

wifi_ret_t NetSTA::start_server() {
    return wifi_ret_t::SERVER_OK;
}

wifi_ret_t NetSTA::destroy() {
    return wifi_ret_t::DESTROY_OK;
}

void NetSTA::setPass(const char* pass) {

}

void NetSTA::setSSID(const char* ssid) {

}

void NetSTA::setPhone(const char* phone) {
    
}

}