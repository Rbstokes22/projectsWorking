#ifndef ALERT_HPP
#define ALERT_HPP

#include <cstdint>
#include "esp_http_client.h"
#include "Threads/Mutex.hpp"
#include "Common/FlagReg.hpp"
#include "UI/MsgLogHandler.hpp"


namespace Peripheral {

#define ALT_JSON_DATA_SIZE 256 // used in send alert
#define REP_JSON_DATA_SIZE 2048 // used in send report to server
#define MSG_RESPONSE_SIZE 16 // used to receive response from server {OK or FAIL}
#define WEB_URL_SIZE 64 // used in prepMsg
#define ALT_CLEANUP_ATTEMPTS 5 // max attempts to clean an http connection.
#define ALT_LOG_METHOD Messaging::Method::SRL_LOG
#define ALT_TAG "(ALERT)"
#define ALT_FLAG_TAG "(ALTflag)"

// 2048 from socket handler buffer size. Cannot include sockethandler due to 
// circular dependencies. 
#define ALT_REPORT_SIZE 2048 + 256 // Should be enough to accomodate the report.

enum class ALTCOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Alert condition.
enum ALTFLAGS : uint8_t {INIT, OPEN};

class Alert {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    char report[ALT_REPORT_SIZE]; // Allows additional space for data.
    static Threads::Mutex mtx;
    Flag::FlagReg flags;
    esp_http_client_config_t config; // http client configuration for alert
    esp_http_client_handle_t client; // http client handle for alert

    Alert();
    Alert(const Alert&) = delete; // prevent copying
    Alert &operator=(const Alert&) = delete; // prevent assignment
    bool prepMsg(const char* jsonData, bool isRep = false);
    bool executeMsg(const char* jsonData, bool isRep = false);
    bool executeWrite(const char* jsonData, bool isRep = false);
    bool executeRead(char* response, size_t size);
    bool initClient();
    bool openClient(const char* jsonData);
    bool cleanup(size_t attempt = 0);
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR);

    public:
    static Alert* get();
    bool sendAlert(const char* msg, const char* caller);
    bool sendReport(const char* JSONmsg);
};

}

#endif // ALERT_HPP