#include "Peripherals/Alert.hpp"
#include <cstdint>
#include "esp_http_client.h"
#include "Threads/Mutex.hpp"
#include "esp_crt_bundle.h"
#include "esp_transport.h"
#include "Config/config.hpp"
#include "string.h"
#include "UI/MsgLogHandler.hpp"
#include "Common/FlagReg.hpp"
#include "Peripherals/saveSettings.hpp"
#include "Network/Handlers/socketHandler.hpp"
#include "Network/NetCreds.hpp"

namespace Peripheral {

Threads::Mutex Alert::mtx(ALT_TAG); // Define static mutex instance.

Alert::Alert() : tag(ALT_TAG), flags(ALT_FLAG_TAG) {

    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
} 

// Returns pointer to Alert instance.
Alert* Alert::get() {

    // Single use of mutex lock which will ensure to protect any subsequent
    // calls made after requesting this instance.
    Threads::MutexLock(Alert::mtx);

    static Alert instance;
    return &instance;
}

// Requires the JSON message and if it is a report, to send to server. Configs
// the certs, headers, and post field. Then executes the messages. Returns true
// upon successful sending, and false if not.
bool Alert::prepMsg(const char* jsonData, bool isRep) {

    esp_err_t err;
    char url[WEB_URL_SIZE] = WEBURL; // URL stored in config.h
    strcat(url, ALERT_PATH); // Append API path to url.

    // Configure POST message and attach default crt bundle. 
    this->config = {
        .url = url,
        .cert_pem = NULL,
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = DEVmode, // false for production
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    // Configure client by initiating client connection. Err hand in func.
    if (!this->initClient()) return false;

    
    if (DEVmode) { // Add message if in development mode, for NGROK only.
        esp_http_client_set_header(this->client, "ngrok-skip-browser-warning", 
            "1");
    }

    // Set the content type to json data. No need to error check, the server
    // is expecting this anyway, despite it being bad etiquette.
    esp_http_client_set_header(this->client, "Content-Type", 
        "application/json");

    // Set the post field to the json data that we are sending to the server.
    err = esp_http_client_set_post_field(this->client, jsonData, 
        strlen(jsonData));

    if (err != ESP_OK) {
        snprintf(this->log, sizeof(this->log), "%s failed to set POST data: %s",
            this->tag, esp_err_to_name(err));

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    // Upon successful prep, execute message.
    return this->executeMsg(jsonData, isRep);
}

// Requires the client, json data, and if report. Opens the connection, writes
// data to the server, reads response, and closes. The anticipated response of 
// the server is "OK" to indicate success, or "FAIL". Returns true if 
// response is "OK", and false if anything other than "OK";
bool Alert::executeMsg(const char* jsonData, bool isRep) {
    char response[MSG_RESPONSE_SIZE]{0}; // Expects OK or FAIL.

    // Ensure when establishing connection, include the payload size here
    // for the following write function, this establishes content length.
    if (!this->openClient(jsonData)) return false; // Err handling in func.
    if (!this->executeWrite(jsonData, isRep)) return false; // Err hand in func.
    if (!this->executeRead(response, sizeof(response))) return false; 
    this->cleanup(); // Final cleanup.

    if (strcmp("OK", response) == 0) { // server received.
        snprintf(this->log, sizeof(this->log), "%s sent", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);
        return true;
    } 

    // Server did not receive.
    snprintf(this->log, sizeof(this->log), "%s not sent", this->tag);
    this->sendErr(this->log);
    return false;
}

// Requires the json data, and if report. Executes the write to the client.
// Returns true if successful, and false if not.
bool Alert::executeWrite(const char* jsonData, bool isRep) {
    // Writes the json data to the http connection.
    int written = esp_http_client_write(this->client, jsonData, 
        strlen(jsonData));

    size_t maxSize = isRep ? REP_JSON_DATA_SIZE : ALT_JSON_DATA_SIZE;

    if (written >= maxSize) { // Sends, but logs exceeding length.
        snprintf(this->log, sizeof(this->log), 
            "%s write size %d exceeds max %d", this->tag, written, 
            ALT_JSON_DATA_SIZE);

        this->sendErr(this->log);

    } else if (written == 0) { // nothing written, so an error for us.
        snprintf(this->log, sizeof(this->log), "%s msg not written to server",
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;

    } else if (written < 0) { // Error at -1
        snprintf(this->log, sizeof(this->log), "%s err writing msg to server",
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    return true; // Write OK.
}

// Requires response pointer to populate with response, and its size. Reads
// http response checking of "OK" or "FAIL" to indicate success. Returns true
// upon success, and false upon failure.
bool Alert::executeRead(char* response, size_t size) {

    // Get content length from the response headers.

    // Content Length is separate than read length. Content length will come
    // from response header, and read length will ensure the length of the
    // data read from the connection. This is separate for error handling.
    int64_t contentLen = esp_http_client_fetch_headers(client);
    int readLen = 0; // length of data read from connection.

    if (contentLen == 0) { // No content 
        snprintf(this->log, sizeof(this->log), 
            "%s no content received from server", this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;

    } else if (contentLen < 0) { // Indicates error at -1
        snprintf(this->log, sizeof(this->log), 
            "%s err recv content from server", this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    // readSize is set to the smaller of the two. This is because the content 
    // is expected to be no larger than 4 chars "FAIL", but in the event it 
    // exceeds the size of the response, it will default at that size but 
    // still respond false.
    size_t maxSize = size - 1; // room for null term
    size_t readSize = contentLen >= maxSize ? maxSize : contentLen;

    // Acct for null terminator with readSize
    readLen = esp_http_client_read(this->client, response, readSize); 

    if (readLen == 0) { // Handle zero chars read

        snprintf(this->log, sizeof(this->log), "%s Response zero chars", 
            this->tag);

        this->sendErr(this->log);
        this->cleanup();
        return false;

    } else if (readLen < 0) { // Error at -1

        snprintf(this->log, sizeof(this->log), "%s Read error", this->tag);
        this->sendErr(this->log);
        this->cleanup();
        return false;
    }

    response[readSize] = '\0'; // null term the last char to be safe
    return true;
}

// Requires no params. Initiates client connection to server if not already
// init. Returns false of not-init, or true if init.
bool Alert::initClient() {

    if (!this->flags.getFlag(ALTFLAGS::INIT)) {
        this->client = esp_http_client_init(&this->config);

        if (this->client == NULL) { // Check for proper init.
            snprintf(this->log, sizeof(this->log), 
                "%s connection unable to init", this->tag);

            this->sendErr(this->log);
            this->cleanup();
            return false;
        }

        this->flags.setFlag(ALTFLAGS::INIT);
    }

    return true;
}

// Requires pointer to json data to open connection with content length.
// Opens the connection with the client, returning true if open and false if 
// not.
bool Alert::openClient(const char* jsonData) {

    if (!this->flags.getFlag(ALTFLAGS::OPEN)) {

        if (esp_http_client_open(client, strlen(jsonData)) != ESP_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s connection unable to open", this->tag);

            this->sendErr(this->log);
            this->cleanup();
            return false;
        }

        this->flags.setFlag(ALTFLAGS::OPEN);
    }

    return true;
}

// Requires attempt number of cleanup. Defaults to 0. This has an internal
// handling system that will retry cleanups for failures, ultimately returning
// true if successful or false if not. Closes and cleans all connections
// iff their specific flag has been set to true during the process. WARNING:
// attempts should only be set within this function to retry another attempt. 
// All fresh cleanup calls should be 0.
bool Alert::cleanup(size_t attempt) {
    attempt++; // Increment the passed attempt, this is to handle retries.

    if (attempt >= ALT_CLEANUP_ATTEMPTS) {

        snprintf(this->log, sizeof(this->log), 
            "%s Unable to close connection, restarting", this->tag);

        this->sendErr(this->log, Messaging::Levels::CRITICAL);
        NVS::settingSaver::get()->save(); // Save peripheral settings

        esp_restart(); // Restart esp attempt.

        snprintf(this->log, sizeof(this->log), // Log if failure.
            "%s Unable to restart", this->tag);

        this->sendErr(this->log, Messaging::Levels::CRITICAL);
        return false; // Just in case it doesnt restart.
    }

    // Default to ESP_OK in event any of the flags are not set here.
    esp_err_t close{ESP_OK}, cleanup{ESP_OK};

    // Check client open. OTA will have to be closed before invoking this.
    if (this->flags.getFlag(ALTFLAGS::OPEN)) {

        close = esp_http_client_close(this->client);

        if (close != ESP_OK) {
            snprintf(this->log, sizeof(this->log), "%s unable to close client",
                this->tag);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            this->cleanup(attempt); // Retry.
        }

        this->flags.releaseFlag(ALTFLAGS::OPEN);
    }

    // Finally check init. Client will need to be closed before invoking this.
    if (this->flags.getFlag(ALTFLAGS::INIT)) {

        cleanup = esp_http_client_cleanup(this->client);

        if (cleanup != ESP_OK) {
            snprintf(this->log, sizeof(this->log), 
                "%s unable to cleanup client",this->tag);

            this->sendErr(this->log);
            vTaskDelay(pdMS_TO_TICKS(50)); // Delay before retry.
            this->cleanup(attempt); // retry.
        }

        this->flags.releaseFlag(ALTFLAGS::INIT);
    }

    return (close == ESP_OK && cleanup == ESP_OK);
}

// Requires message and messaging level. Level default to ERROR.
void Alert::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, ALT_LOG_METHOD);
}

// Requires message and caller. Generates a POSt request in JSON format and
// sends to the server. Ensure that the server responds with "OK" or "FAIL"
// depending on success. If "OK" returns true, returns false if not "OK".
bool Alert::sendAlert(const char* msg, const char* caller) {

    NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq(); 
    if (sms == nullptr) return false; // block to prevent use.

    // Creates JSON from passed arguments and sets write length for headers.
    char jsonData[ALT_JSON_DATA_SIZE] = {0}; // Should be plenty large.
    int written = snprintf(jsonData, sizeof(jsonData), 
        "{\"APIkey\":\"%s\",\"phone\":\"%s\",\"msg\":\"%s\"}",
        sms->APIkey, sms->phone, msg);

    // Ensures that the appropriate amount of data is written.
    if (written < 0 || written > ALT_JSON_DATA_SIZE) return false;

    snprintf(this->log, sizeof(this->log), "%s sending Alt from %s", this->tag,
        caller);
        
    this->sendErr(this->log, Messaging::Levels::INFO);

    return this->prepMsg(jsonData);
}

// Requires message of json type that have KV pairs of the average type, such
// as tempCAvg. Ensure that server response with "OK" or "FAIL" depending on 
// success. If "OK", returns true, if anything else, returns false.
bool Alert::sendReport(const char* JSONmsg) {

    NVS::SMSreq* sms = NVS::Creds::get()->getSMSReq(); 
    if (sms == nullptr) return false; // block to prevent use.

    const char* jsonPrep = 
        "{\"APIkey\":\"%s\",\"phone\":\"%s\",\"report\":%s}";

    // !!!Had stack overflow issues creating a local variable. Created a report
    // class var to omit the requirement for upping the stack size upon init.

    // Create JSON from passed arguments and set the write length for headers.
    // char jsonData[adjustedSize] = {0}; // Large, causes impact on routineTask.
    int written = snprintf(this->report, sizeof(this->report), jsonPrep, 
        sms->APIkey, sms->phone, JSONmsg);

    // Ensure that the appropriate amount of data is written.
    if (written < 0 || written > sizeof(this->report)) return false;

    snprintf(this->log, sizeof(this->log), "%s sending report", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);

    return this->prepMsg(this->report, true);
}
    
}