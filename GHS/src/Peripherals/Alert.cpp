#include "Peripherals/Alert.hpp"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_transport.h"
#include "Config/config.hpp"
#include "string.h"

namespace Peripheral {

Alert::Alert() {} // No def

// Returns pointer to Alert instance.
Alert* Alert::get() {
    static Alert instance;
    return &instance;
}

// Requires 8-char API key, 10 Digit Phone, and message. Generates a POST 
// request in JSON format and sends to the server. Returns true if successful
// and false if not.
bool Alert::sendMessage(
    const char* APIkey,
    const char* phone,
    const char* msg
    ) {

    esp_err_t err;

    char url[100] = WEBURL;
    strcat(url, ALERT_PATH); // Append API path.

    int written = 0;

    // Creates JSON from passed arguments.
    char jsonData[256](0); // Should be plenty large for this purpose.
    written = snprintf(jsonData, sizeof(jsonData), 
        "{\"APIkey\":\"%s\",\"phone\":\"%s\",\"msg\":\"%s\"}",
        APIkey, phone, msg);

    // Ensures that the appropriate amount of data is written.
    if (written < 0 || written > 256) return false;

    // Configure POST message
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (DEVmode) {
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");

    err = esp_http_client_set_post_field(client, jsonData, strlen(jsonData));

    if (err != ESP_OK) {
        printf("Failed to set POST Data: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    // Perform the POST request
    err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        printf("POST Status: %d, content-len: %lld\n",
        esp_http_client_get_status_code(client),
        esp_http_client_get_content_length(client));
    } else {
        printf("Error, POST request failed\n");
    }

    esp_http_client_cleanup(client);
    return (err == ESP_OK);
}
    
}