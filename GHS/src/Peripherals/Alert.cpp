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

// Requires the json message to send to the server. Configures the the certs,
// headers, and post field, and then executes the message. Returns true upon a
// successful send, and false of unsuccessful attempt.
bool Alert::prepMsg(const char* jsonData) {

    esp_err_t err;
    char url[100] = WEBURL; // URL stored in config.h
    strcat(url, ALERT_PATH); // Append API path to url.

    // Configure POST message and attach default crt bundle. 
    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    // Configure client.
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    if (DEVmode) { // Add message if in development mode, for NGROK only.
        esp_http_client_set_header(client, "ngrok-skip-browser-warning", "1");
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");

    err = esp_http_client_set_post_field(client, jsonData, strlen(jsonData));

    if (err != ESP_OK) {
        printf("Alert: Failed to set POST Data: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    // Upon successful prep, execute message.
    return this->executeMsg(client, jsonData);
}

// Requires the client, and json data. Opens the connection, writes json data
// to the server, reads response, and closes. The anticipated response of 
// the server is "OK" to indicate success, or "FAIL". Returns true if 
// response is "OK", and false if anything other than "OK";
bool Alert::executeMsg(esp_http_client_handle_t client, const char* jsonData) {
    char response[MSG_RESPONSE_SIZE]{0}; // Expects OK or FAIL.
    esp_err_t err;

    // Content Length is separate than read length. Content length will come
    // from response header, and read length will ensure the length of the
    // data read from the connection. This is separate for error handling.
    int64_t contentLen = 0; // content length from response header.
    int readLen = 0; // length of data read from connection.
    int written = 0; // length of data written to connection.

    // Ensure when establishing connection, include the payload size here
    // for the following write function.
    err = esp_http_client_open(client, strlen(jsonData)); 

    if (err != ESP_OK) {
        printf("Alert: Failed to establish connection with client\n");
        esp_http_client_cleanup(client);
        return false;
    }

    // Writes the json data to the http connection.
    written = esp_http_client_write(client, jsonData, strlen(jsonData));

    if (written >= sizeof(jsonData)) {
        printf("Alert wrt: %d exceeds size: %d\n", written, sizeof(jsonData));

    } else if (written <= 0) { // Indiactes no read or error.
        printf("Alert: message not written to server or error\n");
        esp_http_client_cleanup(client);
        return false;
    }

    // Extracted from response headers.
    contentLen = esp_http_client_fetch_headers(client);

    if (contentLen <= 0) { // No content or error.
        printf("Alert: Receive no content from server or error\n");
        esp_http_client_cleanup(client);
        return false;
    }

    // readSize is set to the smaller of the two. This is because the content 
    // is expected to be no larger than 4 chars "FAIL", but in the event it 
    // exceeds the size of the response, it will default at that size but 
    // still respond false.
    size_t maxSize = sizeof(response) - 1;
    size_t readSize = contentLen >= maxSize ? maxSize : contentLen;

    // Acct for null terminator with readSize
    readLen = esp_http_client_read(client, response, readSize); 

    if (readLen <= 0) {
        printf("Alert: Response zero or error\n");
        esp_http_client_cleanup(client);
        return false;
    }

    response[readSize] = '\0'; // null term the last char

    esp_http_client_close(client);    
    esp_http_client_cleanup(client);

    return (strcmp("OK", response) == 0); // true if returns ok, false if not.
}

// Requires 8-char API key, 10 Digit Phone, and message. Generates a POST 
// request in JSON format and sends to the server. Ensure that the server
// responds with "OK" or "FAIL" depending on the success. If "OK", returns
// true, if anything else, returns false.
bool Alert::sendAlert(const char* APIkey, const char* phone, const char* msg) {

    // Creates JSON from passed arguments and sets write length for headers.
    char jsonData[ALT_JSON_DATA_SIZE] = {0}; // Should be plenty large.
    int written = snprintf(jsonData, sizeof(jsonData), 
        "{\"APIkey\":\"%s\",\"phone\":\"%s\",\"msg\":\"%s\"}",
        APIkey, phone, msg);

    // Ensures that the appropriate amount of data is written.
    if (written < 0 || written > ALT_JSON_DATA_SIZE) return false;

    return this->prepMsg(jsonData);
}

// Requires message of json type that have KV pairs of the average type, such
// as tempCAvg. Ensure that server response with "OK" or "FAIL" depending on 
// success. If "OK", returns true, if anything else, returns false.
bool Alert::sendAverages(const char* JSONmsg) {

    // Create JSON from passed arguments and set the write length for headers.
    char jsonData[AVG_JSON_DATA_SIZE] = {0}; // size should suffice
    int written = snprintf(jsonData, sizeof(jsonData),
        "{\"averages\":\"%s\"}", JSONmsg);

    // Ensure that the appropriate amount of data is written.
    if (written < 0 || written > AVG_JSON_DATA_SIZE) return false;

    return this->prepMsg(jsonData);
}
    
}