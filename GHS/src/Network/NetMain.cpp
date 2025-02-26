#include "Network/NetMain.hpp"
#include "Config/config.hpp"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "UI/MsgLogHandler.hpp"
#include <cstddef>
#include "string.h"
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
httpd_handle_t NetMain::server{NULL};
NetMode NetMain::NetType{NetMode::NONE};
Flag::FlagReg NetMain::flags; 
esp_netif_t* NetMain::ap_netif{nullptr};
esp_netif_t* NetMain::sta_netif{nullptr};
wifi_init_config_t NetMain::init_config = WIFI_INIT_CONFIG_DEFAULT();
httpd_config_t NetMain::http_config = HTTPD_DEFAULT_CONFIG();

NetMain::NetMain(const char* mdnsName) : tag("Net"), wifi_config{{}} {

    size_t mdnsLen = sizeof(this->mdnsName);

    strncpy(this->mdnsName, mdnsName, mdnsLen - 1);
    this->mdnsName[mdnsLen - 1] = '\0';

    // used for https client due to large size.
    NetMain::http_config.stack_size = 8192; 
}


NetMain::~NetMain() {} // virtual destroyer

// Must be first step in process.
// Requires no params. Configs upon network startup allowing subclasses to
// dynamically change config on an already running system. Returns INIT_OK and
// INIT_FAIL
wifi_ret_t NetMain::init_wifi() {

    // Initializes the wifi driver.
    if (!NetMain::flags.getFlag(wifiInit)) {
        esp_err_t wifi_init = esp_wifi_init(&NetMain::init_config);
        
        if (wifi_init == ESP_OK) {
            NetMain::flags.setFlag(wifiInit);
        } else {
            this->sendErr("Wifi init Fail");
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Initializes the network interface.
    if (!NetMain::flags.getFlag(netifInit)) {
        esp_err_t netif_init = esp_netif_init();
        
        if (netif_init == ESP_OK) {
            NetMain::flags.setFlag(netifInit);
        } else {
            this->sendErr("Netif unable to init");;
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Creates the default event loop.
    if (!NetMain::flags.getFlag(eventLoopInit)) {
        esp_err_t event_loop = esp_event_loop_create_default();
        
        if (event_loop == ESP_OK) {
            NetMain::flags.setFlag(eventLoopInit);
        } else {
            this->sendErr("Unable to create eventloop");
            return wifi_ret_t::INIT_FAIL; 
        }
    }

    // Creates the wireless access point network interface and loads into ptr.
    if (!NetMain::flags.getFlag(ap_netifCreated)) {
        
        if (NetMain::ap_netif == nullptr) {
            
            NetMain::ap_netif = esp_netif_create_default_wifi_ap();

            if (NetMain::ap_netif == nullptr) {
                this->sendErr("ap_netif not set");
            } else {
                NetMain::flags.setFlag(ap_netifCreated);
            }
        }
    }

    // Creates the station network interface and loads into ptr.
    if (!NetMain::flags.getFlag(sta_netifCreated)) {
        
        if (NetMain::sta_netif == nullptr) {

            NetMain::sta_netif = esp_netif_create_default_wifi_sta();

            if (NetMain::sta_netif == nullptr) {
                this->sendErr("sta_netif not set");
            } else {
                NetMain::flags.setFlag(sta_netifCreated);
            }
        }
    }

    return wifi_ret_t::INIT_OK;
}

// Fourth step of init process. Included here since it is a shared resource.
// Steps 2 and 3 are with the subclasses. Creates a multicast DNS that is 
// discoverable instead of relying on ip. 
wifi_ret_t NetMain::mDNS() { 

    // inits the mdns.
    if (!NetMain::flags.getFlag(mdnsInit)) {
        
        if (mdns_init() == ESP_OK) {
            NetMain::flags.setFlag(mdnsInit);
        } else {
            this->sendErr("mDNS Init Failed");
            return wifi_ret_t::MDNS_FAIL;
        }
    }
    
    // sets the mdns hostname.
    if (!NetMain::flags.getFlag(mdnsHostSet)) {
        
        if (mdns_hostname_set(this->mdnsName) == ESP_OK) {
            NetMain::flags.setFlag(mdnsHostSet);
        } else {
            this->sendErr("mDNS Hostname not set");
            return wifi_ret_t::MDNS_FAIL;
        }
    }
  
    // then sets the mdns instance name.
    if (!NetMain::flags.getFlag(mdnsInstanceSet)) {
        
        char insName[sizeof(this->mdnsName) + strlen(" Device") + 2]; // +2 pad
        snprintf(insName, sizeof(insName), "%s Device", this->mdnsName);

        if (mdns_instance_name_set(insName) == ESP_OK) {
            NetMain::flags.setFlag(mdnsInstanceSet);
        } else {
            this->sendErr("mDNS instance name not set");
            return wifi_ret_t::MDNS_FAIL;
        }
    } 

    // and finally adds the mdns service.
    if (!NetMain::flags.getFlag(mdnsServiceAdded)) {
        
        esp_err_t svcAdd = mdns_service_add(
            this->mdnsName, "_http._tcp", "_tcp", 
            80, NULL, 0);

        if (svcAdd == ESP_OK) {
            NetMain::flags.setFlag(mdnsServiceAdded);
            
        } else {
            this->sendErr("mDNS service not added");
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

// Calculates the remaning heap size for display. Accepts the size B, KB, or 
// MB, and returns float of remaning size.
float NetMain::getHeapSize(HEAP_SIZE type) { 
    size_t freeHeapBytes = esp_get_free_heap_size();
    
    uint32_t divisor[]{1, 1000, 1000000}; // Used to compute appropriate units.

    return static_cast<float>(freeHeapBytes) / divisor[static_cast<int>(type)];
}

// Used in web sockets to check the server for active ws clients.
httpd_handle_t NetMain::getServer() {
    return this->server;
}

// Requires message and messaging level which is set to ERROR in default.
// Prints message to log and serial. Ensure passed message is <= 120 bytes.
void NetMain::sendErr(const char* msg, Messaging::Levels lvl) {

    char log[LOG_MAX_ENTRY]{0}; // Log to be used to combine tag and msg.
    snprintf(log, sizeof(log), "%s: %s", this->tag, msg);

    Messaging::MsgLogHandler::get()->handle(lvl, log, 
            Messaging::Method::SRL_LOG);
}

}