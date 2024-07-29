#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstddef>

namespace Comms {

// Static Setup
NetMode NetMain::NetType{NetMode::NONE};
bool NetMain::isWifiInit{false};

NetMain::NetMain(Messaging::MsgLogHandler &msglogerr) : 
    server{NULL}, msglogerr(msglogerr) {

        if (!NetMain::isWifiInit) {
            this->init_wifi_once();
        }
    }

NetMain::~NetMain() {}

NetMode NetMain::getNetType() {
    return NetMain::NetType;
}

void NetMain::setNetType(NetMode NetType) {
    NetMain::NetType = NetType;
}

float NetMain::getHeapSize() {
    static size_t totalHeap = esp_get_minimum_free_heap_size();
    size_t freeHeap = esp_get_free_heap_size();

    return static_cast<float>(freeHeap) / totalHeap;
}

void NetMain::sendErr(const char* msg) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::OLED, 
        Messaging::Method::SRL);
}

wifi_ret_t NetMain::init_wifi_once() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t wifi_init = esp_wifi_init(&cfg);

    if (wifi_init != ESP_OK) {
        this->sendErr("WIFI not configured");
        return wifi_ret_t::CONFIG_FAIL;
    } else {
        NetMain::isWifiInit = true;
        return wifi_ret_t::CONFIG_OK;
    }
}

}