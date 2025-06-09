#include "Network/NetMain.hpp"
#include "Config/config.hpp"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "UI/MsgLogHandler.hpp"
#include "driver/gpio.h"
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

// Flow 
// Func 1 -> step 1, step 2, ... step n. Func 2 when func 1 is complete.
// Func 2 -> step 1, step 2, ..., step n. Func n when func 2 is complete.
// Func n -> step 1, step 2, ..., step n. Once complete, connection is up.
// If any steps fail within a function, it will attempt again, leaving the
// failing handler, the next up for a retry, considering all flags are set to
// true indicating the previous steps are already init. Upon destroy, all flags
// are reset.

namespace Comms {

// Static Setup
httpd_handle_t NetMain::server{NULL};
NetMode NetMain::NetType{NetMode::NONE};
Flag::FlagReg NetMain::flags("(NetFlag)"); 
esp_netif_t* NetMain::ap_netif{nullptr};
esp_netif_t* NetMain::sta_netif{nullptr};
wifi_init_config_t NetMain::init_config = WIFI_INIT_CONFIG_DEFAULT();
httpd_config_t NetMain::http_config = HTTPD_DEFAULT_CONFIG();
char NetMain::log[LOG_MAX_ENTRY]{0};

NetMain::NetMain(const char* mdnsName) : 
    
    tag("(NetMain)"), logToggle{true, true}, wifi_config{{}} {

    size_t mdnsLen = sizeof(this->mdnsName);

    strncpy(this->mdnsName, mdnsName, mdnsLen - 1);
    this->mdnsName[mdnsLen - 1] = '\0';

    // used for https client due to large size.
    NetMain::http_config.stack_size = STACK_SIZE; 

    // Abstract class, no object will be created. Subclasses will log
    // their creation.
}

NetMain::~NetMain() {} // virtual destroyer required

// Must be first step in process.
// Requires no params. Configs upon network startup allowing subclasses to
// dynamically change config on an already running system. Returns INIT_OK and
// INIT_FAIL
wifi_ret_t NetMain::init_wifi() {

    // Initializes the wifi driver.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiInit)) {
        esp_err_t wifi_init = esp_wifi_init(&NetMain::init_config);
        
        if (wifi_init == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiInit);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi init Fail: %s", this->tag, 
                esp_err_to_name(wifi_init));

            this->sendErr(NetMain::log);
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Initializes the network interface.
    if (!NetMain::flags.getFlag(NETFLAGS::netifInit)) {
        esp_err_t netif_init = esp_netif_init();
        
        if (netif_init == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::netifInit);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Netif init fail: %s", this->tag, 
                esp_err_to_name(netif_init));
                
            this->sendErr(NetMain::log);
            return wifi_ret_t::INIT_FAIL;
        }
    }

    // Creates the default event loop.
    if (!NetMain::flags.getFlag(NETFLAGS::eventLoopInit)) {
        esp_err_t event_loop = esp_event_loop_create_default();
        
        if (event_loop == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::eventLoopInit);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Eventloop fail: %s", this->tag, 
                esp_err_to_name(event_loop));
                
            this->sendErr(NetMain::log);
            return wifi_ret_t::INIT_FAIL; 
        }
    }

    // Creates the wireless access point network interface and loads into ptr.
    if (!NetMain::flags.getFlag(NETFLAGS::ap_netifCreated)) {
        
        if (NetMain::ap_netif == nullptr) {
            
            NetMain::ap_netif = esp_netif_create_default_wifi_ap();

            if (NetMain::ap_netif == nullptr) {
                snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s AP Netif not set", this->tag);

                this->sendErr(NetMain::log);
                return wifi_ret_t::INIT_FAIL;

            } else {
                NetMain::flags.setFlag(NETFLAGS::ap_netifCreated);
            }
        }
    }

    // Creates the station network interface and loads into ptr.
    if (!NetMain::flags.getFlag(NETFLAGS::sta_netifCreated)) {
        
        if (NetMain::sta_netif == nullptr) {

            NetMain::sta_netif = esp_netif_create_default_wifi_sta();

            if (NetMain::sta_netif == nullptr) {
                snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s STA Netif not set", this->tag);

                this->sendErr(NetMain::log);
                return wifi_ret_t::INIT_FAIL;

            } else {
                NetMain::flags.setFlag(NETFLAGS::sta_netifCreated);
            }
        }
    }

    return wifi_ret_t::INIT_OK;
}

// Fourth step of init process. Included here since it is a shared resource.
// Steps 2 and 3 are with the subclasses. Creates a multicast DNS that is 
// discoverable instead of relying on ip. 
wifi_ret_t NetMain::mDNS() { 

    // Run a check of the GPIO pins 19, 18, and 5 for binary representation for
    // the instance that several devices are ran on a single network. Allows
    // MDNS to read greenhouse.local to greenhouse7.local, giving 8 options.
    // Used the not operator, to negate the input pullup. LOW signals selection.
    uint8_t b0 = !gpio_get_level( // bit-0 LSB
        CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::MD0)]);

    uint8_t b1 = !gpio_get_level( // bit-1
        CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::MD1)]);

    uint8_t b2 = !gpio_get_level( // bit-2
        CONF_PINS::pinMapD[static_cast<uint8_t>(CONF_PINS::DPIN::MD2)]);

    uint8_t mdnsVal = b0; mdnsVal |= (b1 << 1); mdnsVal |= (b2 << 2);

    if (mdnsVal > 0 && mdnsVal < 8) { // Indicates mdns change. Appends num val.

        // MDNS is set to 16 chars, using greenhouse, this leaves plenty of
        // padding to append a number string to that, especially single digit.
        char temp[strlen(this->mdnsName) + 2]; 
        snprintf(temp, sizeof(temp), "%s%u", this->mdnsName, mdnsVal);

        if (strlen(temp) <= sizeof(this->mdnsName) - 1) { // null char
            snprintf(this->mdnsName, sizeof(this->mdnsName), "%s", temp);
        }
    }

    // inits the mdns.
    if (!NetMain::flags.getFlag(NETFLAGS::mdnsInit)) {
        esp_err_t mdns = mdns_init();

        if (mdns == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::mdnsInit);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s mDNS init fail: %s", this->tag, esp_err_to_name(mdns));
                
            this->sendErr(NetMain::log);
            return wifi_ret_t::MDNS_FAIL;
        }
    }
    
    // sets the mdns hostname.
    if (!NetMain::flags.getFlag(NETFLAGS::mdnsHostSet)) {
        esp_err_t mdns_set = mdns_hostname_set(this->mdnsName);

        if (mdns_set == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::mdnsHostSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s mDNS hostname unset: %s", this->tag, 
                esp_err_to_name(mdns_set));
                
            this->sendErr(NetMain::log);
            return wifi_ret_t::MDNS_FAIL;
        }
    }
  
    // then sets the mdns instance name.
    if (!NetMain::flags.getFlag(NETFLAGS::mdnsInstanceSet)) {
        
        // instance name
        char insName[sizeof(this->mdnsName) + strlen(" Device") + 2]; // +2 pad
        snprintf(insName, sizeof(insName), "%s Device", this->mdnsName);

        esp_err_t mdns_nameset = mdns_instance_name_set(insName);

        if (mdns_nameset == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::mdnsInstanceSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s mDNS instance name unset: %s", this->tag, 
                esp_err_to_name(mdns_nameset));
                
            this->sendErr(NetMain::log);
            return wifi_ret_t::MDNS_FAIL;
        }
    } 

    // and finally adds the mdns service.
    if (!NetMain::flags.getFlag(NETFLAGS::mdnsServiceAdded)) {
        
        esp_err_t svcAdd = mdns_service_add(
            this->mdnsName, "_http._tcp", "_tcp", 
            80, NULL, 0);

        if (svcAdd == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::mdnsServiceAdded);
            
        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s mDNS service not added: %s", this->tag, 
                esp_err_to_name(svcAdd));
                
            this->sendErr(NetMain::log);
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

// Requires message, message level, and if repeating log analysis should be 
// ignored. Messaging default to ERROR, ignoreRepeat default to false.
void NetMain::sendErr(const char* msg, Messaging::Levels lvl, 
    bool ignoreRepeat) {

    Messaging::MsgLogHandler::get()->handle(lvl, msg, 
        NET_LOG_METHOD, ignoreRepeat);
}

// Returns the log toggle struct ptr for reading and writing.
LogToggle* NetMain::getLogToggle() {return &this->logToggle;}

}