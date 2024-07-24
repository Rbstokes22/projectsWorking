#include "Network/NetWAP.hpp"
#include "Network/Routes.hpp"

// ADD LOGIC TO DIFFERENTIATE BETWEEN WAP AND WAP_SETUP USE CREDS IN NET MANAGER
// TO PASS DATA HERE

namespace Communications {

NetWAP::NetWAP(Messaging::MsgLogHandler &msglogerr) : 
    NetMain(msglogerr) {}

void NetWAP::init_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void NetWAP::start_wifi() {
    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

    IP4_ADDR(&ip_info.ip, 192,168,1,1);
    IP4_ADDR(&ip_info.gw, 192,168,1,1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, "GREENHOUSE");
    wifi_config.ap.ssid_len = strlen("GREENHOUSE");
    wifi_config.ap.channel = 6;
    strcpy((char*)wifi_config.ap.password, "12345678");
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    

    if (strlen("12345678") < 8) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN; // No password, open network
    }

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));  // Stop DHCP server first if it was already running
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif)); // Start DHCP server
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); 
}

void NetWAP::start_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&this->server, &config) == ESP_OK) {
        httpd_register_uri_handler(this->server, &WAPSetupIndex);
    } 
}

void NetWAP::destroy() {
    httpd_stop(this->server);
}

}