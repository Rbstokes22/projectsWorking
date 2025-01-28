#ifndef ALERT_HPP
#define ALERT_HPP

#include <cstdint>
#include "esp_http_client.h"

namespace Peripheral {

#define ALT_JSON_DATA_SIZE 256 // used in send alert
#define REP_JSON_DATA_SIZE 512 // used in send report to server
#define MSG_RESPONSE_SIZE 16 // used to receive response from server
#define WEB_URL_SIZE 64 // used in prepMsg

enum class ALTCOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Alert condition.

class Alert {
    private:
    Alert();
    Alert(const Alert&) = delete; // prevent copying
    Alert &operator=(const Alert&) = delete; // prevent assignment
    bool prepMsg(const char* jsonData);
    bool executeMsg(esp_http_client_handle_t client, const char* jsonData);

    public:
    static Alert* get();
    bool sendAlert(const char* APIkey, const char* phone, const char* msg);
    bool sendReport(const char* JSONmsg);
};

}

#endif // ALERT_HPP