#include "OTA/OTAupdates.hpp"
#include "esp_http_client.h"
#include "Network/NetMain.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "string.h"

namespace OTA {

void OTAhandler::writeOTA(esp_http_client_handle_t client, int contentLen) {

    esp_err_t err;
    OLED->setOverrideStat(true);
    OLED->printUpdates("OTA BEGINNING");

    char buffer[1024]{0};
    int dataRead = 0;
    int totalWritten = 0;
    char progress[20]{0};

    // Loop to read data from the HTTP response
    while ((dataRead = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) { 

        // Write the data to the OTA partition
        err = esp_ota_write(this->LANhandle, buffer, dataRead);
        if (err != ESP_OK) {
            OLED->printUpdates("OTA Failed");
            esp_http_client_cleanup(client);
            esp_ota_end(this->LANhandle);
            OLED->setOverrideStat(false);
            return;
        }

        totalWritten += dataRead;
        snprintf(
            progress, 
            sizeof(progress), 
            " %.1f%%", 
            static_cast<float>(totalWritten) * 100 / contentLen);
        OLED->updateProgress(progress);
    }

    if (dataRead < 0) {
        OLED->printUpdates("OTA Failed");
    } else if (dataRead == 0) {
        OLED->printUpdates("OTA Updated, restarting");
    }

    OLED->setOverrideStat(false);
}

// Ensure that the node express server is running and configured to serve
// the correct file required by LAN.
void OTAhandler::updateLAN() {
    esp_err_t err;
  
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        this->sendErr("No OTA partitition found");
        return;
    }

    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return;
    }

    // Configure HTTP client
    char url[50] = LOCAL_SERVER;
    strcat(url, "OTAupdate");

    esp_http_client_config_t config = {
        .url = url
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        printf("Unable to open connection: %s\n", esp_err_to_name(err));
        return;
    }

    int contentLength = esp_http_client_fetch_headers(client);

    err = esp_ota_begin(update_partition, contentLength, &this->LANhandle);
    if (err != ESP_OK) {
        this->sendErr("OTA did not begin");
        return;
    } 

    this->writeOTA(client, contentLength);

    // Clean up and finalize the OTA process
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    esp_ota_end(this->LANhandle);
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
}

// Pings the index page of the local server to check if connected.
// Returns true of false depending on connection.
bool OTAhandler::checkLAN() {
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = LOCAL_SERVER
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        printf("Unable to open connection: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    } else {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return true;
    }
}

}