// #include "OTA/OTAupdates.hpp"
// #include "esp_http_client.h"
// #include "Network/NetMain.hpp"
// #include "esp_ota_ops.h"
// #include "esp_https_ota.h"
// #include "UI/MsgLogHandler.hpp"
// #include "UI/Display.hpp"

// namespace OTA {

// // Static setup

// UI::Display* OTAhandler::OLED = nullptr;
// size_t OTAhandler::firmwareSize{0};

// void OTAhandler::event_handler( // USED FOR DISPLAY ONLY
//     void* arg, esp_event_base_t event_base,
//     int32_t event_id, void* event_data) {

//     char msg[120]{0}; // random choice, adjust in future
//     if (event_base == ESP_HTTPS_OTA_EVENT) {
//         switch (event_id) {
//             case ESP_HTTPS_OTA_START:
//                 OTAhandler::OLED->printUpdates("OTA started");
//                 break;
//             case ESP_HTTPS_OTA_CONNECTED:
//                 OTAhandler::OLED->printUpdates("Connected to server");
//                 break;
//             case ESP_HTTPS_OTA_GET_IMG_DESC:
//                 OTAhandler::OLED->printUpdates("Reading Image Description");
//                 break;
//             case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
//                 memset(msg, 0, sizeof(msg));
//                 snprintf(
//                     msg, 
//                     sizeof(msg) - 1, 
//                     "Verifying chip id of new image: %d", 
//                     *(esp_chip_id_t *)event_data
//                     );

//                 OTAhandler::OLED->printUpdates(msg);
//                 break;
//             case ESP_HTTPS_OTA_DECRYPT_CB:
//                 OTAhandler::OLED->printUpdates("Callback to decrypt function");
//                 break;
//             case ESP_HTTPS_OTA_WRITE_FLASH: 
//                 memset(msg, 0, sizeof(msg)); // THIS MIGHT BE SYSTEM INTENSIVE, RUN TRIAL
//                 snprintf(
//                     msg, 
//                     sizeof(msg) - 1, 
//                     "Writing to flash: %d written", 
//                     *(int *)event_data
//                     );

//                 OTAhandler::OLED->printUpdates(msg);
//                 printf("WRITE GETTING CALLED\n");
//                 break;
//             case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
//                 memset(msg, 0, sizeof(msg));
//                 snprintf(
//                     msg, 
//                     sizeof(msg) - 1, 
//                     "Boot partition updated. Next Partition: %d", 
//                     *(esp_partition_subtype_t *)event_data
//                     );
//                 OTAhandler::OLED->printUpdates(msg);
//                 break;
//             case ESP_HTTPS_OTA_FINISH:
//                 OTAhandler::OLED->printUpdates("OTA finish");
//                 break;
//             case ESP_HTTPS_OTA_ABORT:
//                 OTAhandler::OLED->printUpdates("OTA abort");
//                 break;
//         }
//     }
// }

// void OTAhandler::registerHandler() {
//     esp_err_t err = esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, OTAhandler::event_handler, NULL);

//     if (err != ESP_OK) {
//         printf("Failed to register OTA event handler: %s\n", esp_err_to_name(err));
//     }
// }

// void OTAhandler::unregisterHandler() {
//     esp_err_t err = esp_event_handler_unregister(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, OTAhandler::event_handler);

//     if (err != ESP_OK) {
//         printf("Failed to unregister OTA event handler: %s\n", esp_err_to_name(err));
//     }
// }

// // PUBLIC

// OTAhandler::OTAhandler(
//     UI::Display &OLED, 
//     Comms::NetMain &station,
//     Messaging::MsgLogHandler &msglogerr) :

//     station(station), LANhandle{0}, WEBhandle{nullptr}, msglogerr(msglogerr)

//     {
//         OTAhandler::OLED = &OLED; 
//     }

// // Ensure that the node express server is running and configured to serve
// // the correct file required by LAN.
// void OTAhandler::updateLAN() {
//     esp_err_t err;
//     OLED->setOverrideStat(true);
  
//     const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

//     if (update_partition == NULL) {
//         this->sendErr("No OTA partitition found");
//         return;
//     }

//     Comms::NetMode netMode = station.getNetType();
//     bool netConnected = station.isActive();

//     if (!(netMode == Comms::NetMode::STA && netConnected == true)) {
//         this->sendErr("Not connected to STA mode");
//         return;
//     }

 
//     // Configure HTTP client
//     esp_http_client_config_t config = {
//         .url = LOCAL_SERVER
//         // .event_handler = _http_event_handler,
//     };

    
//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
//         printf("Unable to open connection: %s\n", esp_err_to_name(err));
//         return;
//     }

//     // err = esp_http_client_perform(client);

//     if (err == ESP_OK) {
//         int status_code = esp_http_client_get_status_code(client);
//         // int content_length = esp_http_client_get_content_length(client);
//         int content_length = esp_http_client_fetch_headers(client);
//         printf("STAT CODE: %d, Content Length %d\n", status_code, content_length);

//          // Begin OTA update
//         err = esp_ota_begin(update_partition, content_length, &this->LANhandle);
//         if (err != ESP_OK) {
//             printf("OTA BEGIN FAILED\n");
//             return;
//         }

//         char buffer[1024];
//         int data_read = 0;
//         int total_written = 0;

//         // Loop to read data from the HTTP response
//         while ((data_read = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) { 
            
//             // Write the data to the OTA partition
//             err = esp_ota_write(this->LANhandle, buffer, data_read);
//             if (err != ESP_OK) {
//                 printf("OTA WRITE FAILED\n");
//                 esp_http_client_cleanup(client);
//                 esp_ota_end(this->LANhandle);
//                 OLED->setOverrideStat(false);
//                 return;
//             }
//             total_written += data_read;
//             printf("Total Written %d bytes\n", total_written);
//         }

//         // Check if the read was successful
//         if (data_read < 0) {
//             printf("Error reading data from HTTP response\n");
//         } else if (data_read == 0) {
//             printf("End of steram reached\n");
//         }

//     } else {
//         printf("Unable to setup client\n");
//     }

//     // Clean up and finalize the OTA process
//     esp_http_client_cleanup(client);
//     esp_ota_end(this->LANhandle);
//     this->unregisterHandler();
//     esp_ota_set_boot_partition(update_partition);
//     OLED->setOverrideStat(false);
//     esp_restart();
// }

// void OTAhandler::updateWEB(const char* firmwareURL) {
//     printf("CALLED UPDATE WEB\n");
// }

// void OTAhandler::update(const char* firmwareURL) {
//     // Check LAN first
//     if (this->checkLAN()) {
//         this->updateLAN();
//     } else {
//         this->updateWEB(firmwareURL);
//     }
// }

// // Pings the index page of the local server to check if connected.
// // Returns true of false depending on connection.
// bool OTAhandler::checkLAN() {
//     esp_err_t err;
//     esp_http_client_config_t config = {
//         .url = LOCAL_SERVER
//     };

//     esp_http_client_handle_t client = esp_http_client_init(&config);

//     if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
//         printf("Unable to open connection: %s\n", esp_err_to_name(err));
//         esp_http_client_cleanup(client);
//         return false;
//     } else {
//         esp_http_client_close(client);
//         esp_http_client_cleanup(client);
//         return true;
//     }
// }

// void OTAhandler::sendErr(const char* err) {
//     this->msglogerr.handle(
//         Messaging::Levels::ERROR,
//         err,
//         Messaging::Method::SRL,
//         Messaging::Method::OLED
//     );
// }

// }