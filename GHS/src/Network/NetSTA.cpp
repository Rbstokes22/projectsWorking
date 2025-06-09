#include "Network/NetSTA.hpp"
#include "Network/NetMain.hpp"
#include "esp_wifi.h"
#include "UI/MsgLogHandler.hpp"
#include <cstdint>
#include "Network/Routes.hpp"
#include "string.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mdns.h"
#include <cstddef>

namespace Comms {

// STATIC

// handled within IP event register below.
char NetSTA::IPADDR[static_cast<int>(IDXSIZE::IPADDR)]{0};

// Requires no params. Copies the main wifi configuration with the current
// set ssid and password. Returns CONFIG_OK once complete.
wifi_ret_t NetSTA::configure() { 

    strcpy((char*)this->wifi_config.sta.ssid, this->ssid);
    strcpy((char*)this->wifi_config.sta.password, this->pass); 

    return wifi_ret_t::CONFIG_OK;
}

// Exclusive to station network connection. This event is used to register an
// event handler, which is required to to be able to display the ip address
// that you are connected to, and only that. All parameter requirements are
// inclusively passed.
void NetSTA::IPEvent(void* arg, esp_event_base_t eventBase, int32_t eventID, 
    void* eventData) {

    // Exists exclusively to display IP address on the OLED.
    if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
        esp_netif_ip_info_t ip_info = event->ip_info;

        // Convert IP address and populate the IP address variable.
        esp_ip4addr_ntoa(&ip_info.ip, NetSTA::IPADDR, sizeof(NetSTA::IPADDR));
    }
}

// Constructor, requires mdns name.
NetSTA::NetSTA(const char* mdnsName) : NetMain(mdnsName), tag("(NETSTA)") {

    memset(this->ssid, 0, sizeof(this->ssid));
    memset(this->pass, 0, sizeof(this->pass));
    snprintf(NetMain::log, sizeof(NetMain::log), "%s Ob created", this->tag);
    NetMain::sendErr(NetMain::log, Messaging::Levels::INFO, true);
}

// Second step in the init process, Steps 1 and 4 are in the NetMain src file.
// Requires no params. Registers IP event handler, configures the wifi con,
// sets the wifi mode and configuration, starts the wifi, and connectis to
// the station. Returns WIFI_FAIL or WIFI_OK.
wifi_ret_t NetSTA::start_wifi() {

    // Handles the registration of the event handler.
    if (!NetMain::flags.getFlag(NETFLAGS::handlerReg)) {
        
        esp_err_t handler = esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent, NULL);

        if (handler == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::handlerReg);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Handler Not registered: %s", this->tag, 
                esp_err_to_name(handler));

            this->sendErr(NetMain::log);

            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Always run, no flag. Copies over the ssid and pass to the ip config.
    // Never fails, built like this for future addition if error handling
    // needs to occur within.
    if (this->configure() == wifi_ret_t::CONFIG_FAIL) {
        return wifi_ret_t::WIFI_FAIL; // error prints in configure
    } 

    // Handles the wifiMode set, in this case, station mode will be set.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiModeSet)) {
        
        esp_err_t wifi_mode = esp_wifi_set_mode(WIFI_MODE_STA);

        if (wifi_mode == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiModeSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi mode not set: %s", this->tag, 
                esp_err_to_name(wifi_mode));

            this->sendErr(NetMain::log);
  
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Handles configuration of the wifi settings.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiConfigSet)) {
        
        esp_err_t wifi_cfg = esp_wifi_set_config(WIFI_IF_STA, 
                &this->wifi_config);

        if (wifi_cfg == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiConfigSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi mode not set: %s", this->tag, 
                esp_err_to_name(wifi_cfg));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Handles the start of wifi.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_start = esp_wifi_start();

        if (wifi_start == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi not started: %s", this->tag, 
                esp_err_to_name(wifi_start));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    // Handles the connection to wifi.
    if (!NetMain::flags.getFlag(NETFLAGS::wifiConnected)) {
        
        esp_err_t wifi_con = esp_wifi_connect();

        if (wifi_con == ESP_OK) {
            NetMain::flags.setFlag(NETFLAGS::wifiConnected);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi not connected: %s", this->tag, 
                esp_err_to_name(wifi_con));

            this->sendErr(NetMain::log);
            return wifi_ret_t::WIFI_FAIL;
        }
    }

    return wifi_ret_t::WIFI_OK;
}

// Third step of the init process. 4th step is back to the NetMain src file.
// Once the wifi connection is established, starts the httpd server and 
// registers the Universal Resource Identifiers (URI). Returns SERVER_FAIL
// or SERVER_OK.
wifi_ret_t NetSTA::start_server() {

    // Handles starting the http daemon or httpd server.
    if (!NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
        esp_err_t httpd = httpd_start(&NetMain::server, &NetMain::http_config);

        if (httpd == ESP_OK) {
                NetMain::flags.setFlag(NETFLAGS::httpdOn);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s HTTPD not started: %s", this->tag, esp_err_to_name(httpd));

            this->sendErr(NetMain::log);
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    // Handles registration of the uri handlers. The references passed in the
    // uri handlers are located on Routes header and source.
    if (!NetMain::flags.getFlag(NETFLAGS::uriReg)) {
        
        esp_err_t reg1 = httpd_register_uri_handler(NetMain::server, &STAIndex);
        esp_err_t reg2 = httpd_register_uri_handler(NetMain::server, 
            &OTAUpdate);

        esp_err_t reg3 = httpd_register_uri_handler(NetMain::server, 
            &OTARollback);

        esp_err_t reg4 = httpd_register_uri_handler(NetMain::server, &OTACheck);
        esp_err_t reg5 = httpd_register_uri_handler(NetMain::server, 
            &OTAUpdateLAN);

        esp_err_t reg6 = httpd_register_uri_handler(NetMain::server, &ws);
        esp_err_t reg7 = httpd_register_uri_handler(NetMain::server, &logger);

        // Check to see if all registrations are successful. Must be all or 
        // nothing for return.
        if (reg1 == ESP_OK && reg2 == ESP_OK && reg3 == ESP_OK && reg4 == ESP_OK
            && reg5 == ESP_OK && reg6 == ESP_OK && reg7 == ESP_OK) {
                
            NetMain::flags.setFlag(NETFLAGS::uriReg);
        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s URI registration fail", this->tag);

            this->sendErr(NetMain::log);
            return wifi_ret_t::SERVER_FAIL;
        }
    }

    return wifi_ret_t::SERVER_OK;
}

// Requires no params. Destroys station connection by stopping the mdns and 
// httpd server, stopping wifi, disconnects from the station, and unregisters 
// the IP handler. Resets all required flags back to false, to allow re-init 
// through the init sequence. Returns DESTROY_FAIL or DESTROY_OK.
wifi_ret_t NetSTA::destroy() {

    // Destroys the mdns server.
    if (NetMain::flags.getFlag(NETFLAGS::mdnsInit)) {
        
        mdns_free(); // Stop and free the mdns server.
        NetMain::flags.releaseFlag(NETFLAGS::mdnsInit);
        NetMain::flags.releaseFlag(NETFLAGS::mdnsHostSet);
        NetMain::flags.releaseFlag(NETFLAGS::mdnsInstanceSet);
        NetMain::flags.releaseFlag(NETFLAGS::mdnsServiceAdded);
    }

    // Destroys and stops the httpd connection.
    if (NetMain::flags.getFlag(NETFLAGS::httpdOn)) {
        
        esp_err_t httpd = httpd_stop(NetMain::server);

        if (httpd == ESP_OK) {
            NetMain::flags.releaseFlag(NETFLAGS::httpdOn);
            NetMain::flags.releaseFlag(NETFLAGS::uriReg);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s HTTPD not stopped: %s", this->tag, esp_err_to_name(httpd));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    // Destroys and stops the wifi connection.
    if (NetMain::flags.getFlag(NETFLAGS::wifiOn)) {
        
        esp_err_t wifi_stop = esp_wifi_stop();

        if (wifi_stop == ESP_OK) {
            NetMain::flags.releaseFlag(NETFLAGS::wifiOn);
            NetMain::flags.releaseFlag(NETFLAGS::wifiModeSet);
            NetMain::flags.releaseFlag(NETFLAGS::wifiConfigSet);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi not stopped: %s", this->tag, 
                esp_err_to_name(wifi_stop));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    // Disconnects the wifi connection.
    if (NetMain::flags.getFlag(NETFLAGS::wifiConnected)) {
        
        esp_err_t wifi_con = esp_wifi_disconnect();

        if (wifi_con == ESP_OK || wifi_con == ESP_ERR_WIFI_NOT_STARTED) {
            NetMain::flags.releaseFlag(NETFLAGS::wifiConnected);

        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi not disconnected: %s", this->tag, 
                esp_err_to_name(wifi_con));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    // Unregisters the esp event used for IP conversion to text.
    if (NetMain::flags.getFlag(NETFLAGS::handlerReg)) {
        
        esp_err_t event = esp_event_handler_unregister(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &NetSTA::IPEvent);

        if (event == ESP_OK) {
            NetMain::flags.releaseFlag(NETFLAGS::handlerReg);
            
        } else {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Wifi handler not unregistered: %s", this->tag, 
                esp_err_to_name(event));

            this->sendErr(NetMain::log);
            return wifi_ret_t::DESTROY_FAIL;
        }
    }

    return wifi_ret_t::DESTROY_OK;
}

// Requires the password text string. Sets the password. Max length 64 chars.
void NetSTA::setPass(const char* pass) {

    // Checks that pass is actually passed. Copies pass to object variable.
    if (strlen(pass) != 0 && pass != nullptr) {
        strncpy(this->pass, pass, sizeof(this->pass) - 1);
        this->pass[sizeof(this->pass) - 1] = '\0'; // ensure null term

    } else {
        snprintf(NetMain::log, sizeof(NetMain::log), "%s Pass not set", 
            this->tag);

        this->sendErr(NetMain::log);
    }
}

// Requires the ssid text string. Sets the ssid. Max length is 32 chars.
void NetSTA::setSSID(const char* ssid) {

    // checks that ssid is actually passed. Copies ssid to object variable.
    if (strlen(ssid) != 0 && ssid != nullptr) {
        strncpy(this->ssid, ssid, sizeof(this->ssid) -1);
        this->ssid[sizeof(this->ssid) - 1] = '\0'; // null term

    } else {
        snprintf(NetMain::log, sizeof(NetMain::log), "%s SSID not set", 
            this->tag);

        this->sendErr(NetMain::log);
    }
}

// Requires no parameters. This is pure virtual/abstract and required for 
// the WAP subclass only. Returns password.
const char* NetSTA::getPass(bool defaultPass) const {
    return this->pass;
}

// Requires no parameters. Returns ssid.
const char* NetSTA::getSSID() const {
    return this->ssid;
}

// Requires no parameters. Iterates all station flags. If all are set to true,
// returns true iff station info is populated. Returns false otherwise.
bool NetSTA::isActive() {
    wifi_ap_record_t sta_info; // Station info to be populated.

    const uint8_t flagRequirement = 15; // How many flags to check
    uint8_t flagSuccess = 0;

    // All the required flags to verify active connection.
    bool required[flagRequirement]{
        NetMain::flags.getFlag(NETFLAGS::wifiInit), 
        NetMain::flags.getFlag(NETFLAGS::netifInit),
        NetMain::flags.getFlag(NETFLAGS::eventLoopInit), 
        NetMain::flags.getFlag(NETFLAGS::sta_netifCreated),
        NetMain::flags.getFlag(NETFLAGS::wifiModeSet), 
        NetMain::flags.getFlag(NETFLAGS::wifiConfigSet),
        NetMain::flags.getFlag(NETFLAGS::wifiOn), 
        NetMain::flags.getFlag(NETFLAGS::httpdOn),
        NetMain::flags.getFlag(NETFLAGS::uriReg), 
        NetMain::flags.getFlag(NETFLAGS::handlerReg),
        NetMain::flags.getFlag(NETFLAGS::wifiConnected), 
        NetMain::flags.getFlag(NETFLAGS::mdnsInit),
        NetMain::flags.getFlag(NETFLAGS::mdnsHostSet), 
        NetMain::flags.getFlag(NETFLAGS::mdnsInstanceSet),
        NetMain::flags.getFlag(NETFLAGS::mdnsServiceAdded)
    };

    // Iterate each flag.
    for (int i = 0; i < flagRequirement; i++) {
        flagSuccess += required[i]; // Increments if true
    }

    // Checks that flags have been met and that station info has been acquired.
    // If true, writes a single entry stating connected. If false, also writes
    // a single entry stating disconnected. Each variable is reset upon the 
    // log entry of the opposite condition.
    if ((flagSuccess == flagRequirement) && 
        (esp_wifi_sta_get_ap_info(&sta_info) == ESP_OK)) { // Means connected

        if (this->getLogToggle()->con) {
            snprintf(NetMain::log, sizeof(NetMain::log), 
                "%s Connected", this->tag);

            this->sendErr(NetMain::log, Messaging::Levels::INFO);
            this->getLogToggle()->con = false; // Block another attempt
            this->getLogToggle()->discon = true; // Reset if previously false.
        }

        return true;
        
    } 
    
    // Run this block if not connected
    if (this->getLogToggle()->discon) {
        snprintf(NetMain::log, sizeof(NetMain::log), 
            "%s Disconnected", this->tag);

        this->sendErr(NetMain::log, Messaging::Levels::INFO);
        this->getLogToggle()->discon = false; // Block another attempt
        this->getLogToggle()->con = true; // Reset if previously false.
    }

    return false;
}

// Requires station details reference. Builds the passed detail struct
// containing the ssid, ip address, mdns name, rssi, connection status, and
// free heap memory. 
void NetSTA::getDetails(STAdetails &details) { 
    wifi_ap_record_t sta_info; // Station info that will contain con data.
    
    char status[2][4] = {"No", "Yes"}; // used to display connection status

    // SSID. Doesnt require logging here, that is handled in isActive connect.
    if (esp_wifi_sta_get_ap_info(&sta_info) != ESP_OK) {
        strcpy(details.ssid, "Not Connected"); // Not connected. Copies to ssid.

    } else { // connected, copies the ssid over to the ssid
        strcpy(details.ssid, reinterpret_cast<const char*>(sta_info.ssid));
    }

    // IP. Copies IP address over to the details struct.
    snprintf(details.ipaddr, sizeof(details.ipaddr), "http://%s", 
        NetSTA::IPADDR);

    // MDNS. Copies mDNS over to the details struct.
    char hostname[static_cast<int>(IDXSIZE::MDNS)];
    mdns_hostname_get(hostname);
    snprintf(details.mdns, sizeof(details.mdns), "http://%s.local", hostname);

    // STATUS. Displays "Yes" or "No" depending on activity. 
    strcpy(details.status, status[this->isActive()]);
    
    // RSSI signal strength 
    sprintf(details.signalStrength, "%d dBm", sta_info.rssi);

    // REMAINING HEAP SIZE. Change if using different units.
    sprintf(details.heap, "%.2f KB", this->getHeapSize(HEAP_SIZE::KB));
}

}