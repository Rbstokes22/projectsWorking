#ifndef MASTERHAND_HPP
#define MASTERHAND_HPP

#include "UI/MsgLogHandler.hpp"

namespace Comms {

#define MHAND_LOG_METHOD Messaging::Method::SRL_LOG

// Handles logging and features used throughout all the handlers. The client
// should be accepting these as responses in order to parse.

// Sets the types of responses.
#define MHAND_RESP_TYPE_JSON "application/json"
#define MHAND_RESP_TYPE_TEXTHTML "text/html"

// Responses used to ensure the handlers have been init or not init.
#define MHAND_INIT "{\"status\":\"init\"}" 
#define MHAND_NOT_INIT "{\"status\":\"not init\"}" 

// Responses used to ensure the handlers have set the response type.
#define MHAND_TYPESET_FAIL "{\"status\":\"type not set\"}" // JSON version
#define MHAND_TYPESET_FAIL_TEXT "type not set" // Text version

// Responses used to ensure the client init and connection with the server.
#define MHAND_CON_INIT_FAIL "{\"status\":\"Connection init fail\"}"
#define MHAND_CON_OPEN_FAIL "{\"status\":\"Connection open fail\"}"

// Response used if client received content length is invalid or read error.
#define MHAND_INVAL_CONT_LEN "{\"status\":\"Invalid content Len\"}"
#define MHAND_CLI_READ_ERR "{\"status\":\"Client read err\"}"

// Responses used if OTA attempt is in WAP mode or there is a firmware version 
// match.
#define MHAND_OTA_JSON_WAP "{\"status\":\"wap\"}" 
#define MHAND_OTA_FW_MATCH "{\"status\":\"match\"}" 

// Generic responses to invalid JSON, or a success/fail.
#define MHAND_JSON_INVALD "{\"status\":\"Invalid JSON\"}" // Replies invalid.
#define MHAND_SUCCESS "{\"status\":\"OK\"}"
#define MHAND_FAIL "{\"status\":\"FAIL\"}"

// Response to show a key is not found/permitted in the WAP setup.
#define MHAND_KEY_NOT_FOUND "{\"status\":\"key not found\"}"

// Response to show there is a JSON receive error.
#define MHAND_JSON_RCV_ERR "{\"status\":\"JSON rvc err\"}"

// Responses used after successful WAP setting to show success and/or if a
// WAP reconnection is necessary due to a WAP password change.
#define MHAND_JSON_RECON "{\"status\":\"Accepted: Reconnect to WiFi\"}"
#define MHAND_JSON_ACC "{\"status\":\"Accepted\"}"
#define MHAND_JSON_NOT_ACC "{\"status\":\"Not Accepted\"}"

class MASTERHAND { // Used only for the error handling.
    protected:
    static char log[LOG_MAX_ENTRY];
    static void sendErr(const char* msg, 
        Messaging::Levels lvl = Messaging::Levels::ERROR);

    static bool sendstrErr(esp_err_t err, const char* tag, const char* src);

    public: // None
};

}

#endif // MASTERHAND_HPP