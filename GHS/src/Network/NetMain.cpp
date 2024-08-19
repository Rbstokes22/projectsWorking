#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include <cstddef>
#include "string.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "mdns.h"

// FLAG METHOD and verbosity. Throughout the initialization sequence, 
// there is a lot of verbosity. Each critical piece has its own flag.
// This is by design, and will change the appropriate flag once ran 
// successfully. Return items exist in each piece of logic, which will
// make the first uninitialized component the next in line. This init 
// process will repeat until everything is complete. This method 
// exists throughout the base and sub class methods. Upon successful
// completion of each method, returns an OK.

namespace Comms {

// Static Setup
uint8_t NetMain::certRetries{3};
httpd_handle_t NetMain::server{NULL};
NetMode NetMain::NetType{NetMode::NONE};
Flags NetMain::flags{ // Ensures a linear flow of init process.
    false, false, false, false, false, true, false, false,
    false, false, false, false, false, false, false, false,
    false, false};
esp_netif_t* NetMain::ap_netif{nullptr};
esp_netif_t* NetMain::sta_netif{nullptr};
wifi_init_config_t NetMain::init_config = WIFI_INIT_CONFIG_DEFAULT();
httpd_config_t NetMain::http_config = HTTPD_DEFAULT_CONFIG();

NetMain::NetMain(Messaging::MsgLogHandler &msglogerr, const char* mdnsName) : 

    msglogerr{msglogerr}, wifi_config{{}} {
        size_t mdnsLen = sizeof(this->mdnsName);

        strncpy(this->mdnsName, mdnsName, mdnsLen - 1);
        this->mdnsName[mdnsLen - 1] = '\0';
    }

NetMain::~NetMain() {}

// First step in the init process.
// Configures everything here upon the start allowing the station and wap
// subclasses to simply change configurations dynamically with an already
// running system. Returns INIT_OK or INIT_FAIL.
wifi_ret_t NetMain::init_wifi() {

    // Initializes the wifi driver.
    if (!NetMain::flags.wifiInit) {
        esp_err_t wifi_init = esp_wifi_init(&NetMain::init_config);
        
        if (wifi_init == ESP_OK) {
            NetMain::flags.wifiInit = true;
        } else {
            this->sendErr("Wifi unable to init", errDisp::ALL);
            this->sendErr(esp_err_to_name(wifi_init), errDisp::SRL);
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Initializes the network interface.
    if (!NetMain::flags.netifInit) {
        esp_err_t netif_init = esp_netif_init();
        
        if (netif_init == ESP_OK) {
            NetMain::flags.netifInit = true;
        } else {
            this->sendErr("Netif unable to init", errDisp::ALL);
            this->sendErr(esp_err_to_name(netif_init), errDisp::SRL);
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Creates the default event loop.
    if (!NetMain::flags.eventLoopInit) {
        esp_err_t event_loop = esp_event_loop_create_default();
        
        if (event_loop == ESP_OK) {
            NetMain::flags.eventLoopInit = true;
        } else {
            this->sendErr("Unable to create eventloop", errDisp::ALL);
            this->sendErr(esp_err_to_name(event_loop), errDisp::SRL);
            return wifi_ret_t::INIT_FAIL; 
        }
    }

    // Creates the netif pointer for the WAP connection.
    if (!NetMain::flags.ap_netifCreated) {
        
        if (NetMain::ap_netif == nullptr) {
            
            NetMain::ap_netif = esp_netif_create_default_wifi_ap();

            if (NetMain::ap_netif == nullptr) {
                this->sendErr("ap_netif not set", errDisp::ALL);
            } else {
                NetMain::flags.ap_netifCreated = true;
            }
        }
    }

    // Creates the netif pointer for the STA connection.
    if (!NetMain::flags.sta_netifCreated) {
        
        if (NetMain::sta_netif == nullptr) {

            NetMain::sta_netif = esp_netif_create_default_wifi_sta();

            if (NetMain::sta_netif == nullptr) {
                this->sendErr("ap_netif not set", errDisp::ALL);
            } else {
                NetMain::flags.sta_netifCreated = true;
            }
        }
    }

    return wifi_ret_t::INIT_OK;
}

// Fourth step of init process.
// Creates a DNS discoverable name instead of relying on ip.
// This is essntial for an SSL certificate to be issued.
wifi_ret_t NetMain::mDNS() { 

    if (!NetMain::flags.mdnsInit) {
        
        if (mdns_init() == ESP_OK) {
            NetMain::flags.mdnsInit = true;
        } else {
            this->sendErr("mDNS Init Failed", errDisp::ALL);
            return wifi_ret_t::MDNS_FAIL;
        }
    }
    
    if (!NetMain::flags.mdnsHostSet) {
        
        if (mdns_hostname_set(this->mdnsName) == ESP_OK) {
            NetMain::flags.mdnsHostSet = true;
        } else {
            this->sendErr("mDNS Hostname not set", errDisp::ALL);
            return wifi_ret_t::MDNS_FAIL;
        }
    }
  
    if (!NetMain::flags.mdnsInstanceSet) {
        
        char insName[sizeof(this->mdnsName) + strlen(" Device") + 2]; // +2 for padding
        snprintf(insName, sizeof(insName), "%s Device", this->mdnsName);

        if (mdns_instance_name_set(insName) == ESP_OK) {
            NetMain::flags.mdnsInstanceSet = true;
        } else {
            this->sendErr("mDNS instance name not set", errDisp::ALL);
            return wifi_ret_t::MDNS_FAIL;
        }
    } 

    if (!NetMain::flags.mdnsServiceAdded) {
        
        esp_err_t svcAdd = mdns_service_add(
            this->mdnsName, "_http._tcp", "_tcp", 
            80, NULL, 0);

        if (svcAdd == ESP_OK) {
            NetMain::flags.mdnsServiceAdded = true;
            
        } else {
            this->sendErr("mDNS service not added", errDisp::ALL);
            return wifi_ret_t::MDNS_FAIL;
        }
    }
    
    return wifi_ret_t::MDNS_OK;
}

// Returns the current NetType STA, WAP, WAP_SETUP.
NetMode NetMain::getNetType() {
    return NetMain::NetType;
}

// Sets the current NetType STA, WAP, WAP_SETUP.
void NetMain::setNetType(NetMode NetType) {
    NetMain::NetType = NetType;
}

// Calculates the remaning heap size for display. Accepts the
// size B, KB, or MB, and returns float of remaning size.
float NetMain::getHeapSize(HEAP_SIZE type) { 
    size_t freeHeapBytes = esp_get_free_heap_size();
    
    uint32_t divisor[]{1, 1000, 1000000};

    return static_cast<float>(freeHeapBytes) / divisor[static_cast<int>(type)];
}

// Error handler throughout the network. Accepts a string message,
// and the Type OLED, SRL, or ALL, for display method. This will
// display on the selected method.
void NetMain::sendErr(const char* msg, errDisp type) {

    switch (type) {
        case errDisp::OLED:

        this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::OLED);
        break;

        case errDisp::SRL:
        
        this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::SRL);
        break;

        case errDisp::ALL:

        this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::OLED, 
        Messaging::Method::SRL);
    } 
}

}