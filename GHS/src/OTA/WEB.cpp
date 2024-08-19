#include "OTA/OTAupdates.hpp"
#include "esp_http_client.h"
#include "Network/NetMain.hpp"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "UI/MsgLogHandler.hpp"
#include "UI/Display.hpp"

namespace OTA {

void OTAhandler::event_handler( // USED FOR DISPLAY ONLY
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {

    char msg[120]{0}; // random choice, adjust in future
    if (event_base == ESP_HTTPS_OTA_EVENT) {
        switch (event_id) {
            case ESP_HTTPS_OTA_START:
                OTAhandler::OLED->printUpdates("OTA started");
                break;
            case ESP_HTTPS_OTA_CONNECTED:
                OTAhandler::OLED->printUpdates("Connected to server");
                break;
            case ESP_HTTPS_OTA_GET_IMG_DESC:
                OTAhandler::OLED->printUpdates("Reading Image Description");
                break;
            case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
                memset(msg, 0, sizeof(msg));
                snprintf(
                    msg, 
                    sizeof(msg) - 1, 
                    "Verifying chip id of new image: %d", 
                    *(esp_chip_id_t *)event_data
                    );

                OTAhandler::OLED->printUpdates(msg);
                break;
            case ESP_HTTPS_OTA_DECRYPT_CB:
                OTAhandler::OLED->printUpdates("Callback to decrypt function");
                break;
            case ESP_HTTPS_OTA_WRITE_FLASH: 
                memset(msg, 0, sizeof(msg)); // THIS MIGHT BE SYSTEM INTENSIVE, RUN TRIAL
                snprintf(
                    msg, 
                    sizeof(msg) - 1, 
                    "Writing to flash: %d written", 
                    *(int *)event_data
                    );

                OTAhandler::OLED->printUpdates(msg);
                printf("WRITE GETTING CALLED\n");
                break;
            case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
                memset(msg, 0, sizeof(msg));
                snprintf(
                    msg, 
                    sizeof(msg) - 1, 
                    "Boot partition updated. Next Partition: %d", 
                    *(esp_partition_subtype_t *)event_data
                    );
                OTAhandler::OLED->printUpdates(msg);
                break;
            case ESP_HTTPS_OTA_FINISH:
                OTAhandler::OLED->printUpdates("OTA finish");
                break;
            case ESP_HTTPS_OTA_ABORT:
                OTAhandler::OLED->printUpdates("OTA abort");
                break;
        }
    }
}

void OTAhandler::registerHandler() {
    esp_err_t err = esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, OTAhandler::event_handler, NULL);

    if (err != ESP_OK) {
        printf("Failed to register OTA event handler: %s\n", esp_err_to_name(err));
    }
}

void OTAhandler::unregisterHandler() {
    esp_err_t err = esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, OTAhandler::event_handler);

    if (err != ESP_OK) {
        printf("Failed to unregister OTA event handler: %s\n", esp_err_to_name(err));
    }
}

void OTAhandler::updateWEB(const char* firmwareURL) {
    printf("CALLED UPDATE WEB\n");
}

}