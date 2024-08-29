#include "OTA/OTAupdates.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"
#include "Network/NetMain.hpp"
#include "esp_http_client.h"
#include "string.h"

namespace OTA {

// Pings the index page of the local server to check if connected.
// Returns true of false depending on connection.
bool OTAhandler::checkLAN(const char* firmwareURL) {
    esp_err_t err;
    esp_http_client_config_t config = {
        .url = firmwareURL
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

// If the LAN is active for OTA updates using express server, 
// connects to the server, requests the .bin file, writes to 
// OTA, and closes the connection.
void OTAhandler::updateLAN(const char* firmwareURL) {
    esp_err_t err;
  
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        this->sendErr("No OTA partitition found");
        return;
    }

    // Checks for an active station connection.
    if (!this->isConnected()) {
        this->sendErr("Not connected to network");
        return;
    }

    // Configure HTTP client. Receives the Server URL and 
    // appends it with OTAupdate.
    char url[100]{0};
    sprintf(url, "%s/%s", firmwareURL, "OTAupdate");
  
    esp_http_client_config_t config = {
        .url = url
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        esp_http_client_cleanup(client);
        this->sendErr("Unable to open connection");
        return;
    }

    int contentLength = esp_http_client_fetch_headers(client);

    if (contentLength <= 0) {
        esp_http_client_cleanup(client);
        this->sendErr("Invalid content length");
        return;
    }

    err = esp_ota_begin(update_partition, contentLength, &this->LANhandle);
    if (err != ESP_OK) {
        this->sendErr("OTA did not begin");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    } 

    // Upon successful connection and ota start, writes data to the OTA.
    // If an error is encountered, this will not set the next boot partition.
    if (this->writeOTA(client, contentLength)) {
        esp_ota_set_boot_partition(update_partition);
        esp_restart();
    }
}

// Writes the incoming .bin file to the OTA partition. 
bool OTAhandler::writeOTA(esp_http_client_handle_t client, int contentLen) {
    esp_err_t err;
    OLED->setOverrideStat(true); // Allows progress display.
    OLED->printUpdates("OTA BEGINNING");

    char buffer[1024]{0};
    int dataRead = 0;
    int totalWritten = 0;
    char progress[20]{0};

    auto cleanup = [this, client](const char* msg){
        OLED->printUpdates(msg);
        esp_ota_end(this->LANhandle);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        OLED->setOverrideStat(false);
    };

    // Loop to read data from the HTTP response
    while (true) { 
        dataRead = esp_http_client_read(client, buffer, sizeof(buffer));

        if (dataRead < 0) {
            cleanup("Read Error");
            return false;
        }

        if (dataRead == 0) break;

        // Write the data to the OTA partition
        err = esp_ota_write(this->LANhandle, buffer, dataRead);
        if (err != ESP_OK) {
            cleanup("OTA failed");
            return false;
        }

        totalWritten += dataRead;

        snprintf(
            progress, 
            sizeof(progress), 
            " %.1f%%", 
            static_cast<float>(totalWritten) * 100 / contentLen);
        OLED->updateProgress(progress);
    }

    if (totalWritten == contentLen) {
        cleanup("OTA write success!");
        return true;
    } else {
        cleanup("OTA write fail");
        return false;
    }
}

}