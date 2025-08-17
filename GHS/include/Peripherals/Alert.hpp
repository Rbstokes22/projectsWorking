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

// Alert sensors data. Counts that will trigger a DOWN status alert, and an
// UP status alert.
#define SENS_DOWN_CT 50 // sends sensor DOWN.
#define SENS_UP_CT 10 // sends sensor UP.
#define SENS_SEND_RETRIES 3 // Attempt to send alert.

// 2048 from socket handler buffer size. Cannot include sockethandler due to 
// circular dependencies. 
#define ALT_REPORT_SIZE 2048 + 256 // Should be enough to accomodate the report.

enum class ALTCOND : uint8_t {LESS_THAN, GTR_THAN, NONE}; // Alert condition.
enum ALTFLAGS : uint8_t {INIT, OPEN};

// Used as toggle to prevent several alerts from being sent when cond are met.
enum LAST_SENT : uint8_t {DOWN, UP, NONE};

// Sensor down package is used with sendSensorAlert(); It will be a static 
// record keeper to ensure that both broken and fixed alerts are properly 
// managed for each sensor.
struct SensDownPkg {
    char sensor[32]; // Sensor name.
    bool status; // Is sensor in violation of exceeding count.
    bool prevStatus; // Was the sensor in violation of exceeding count.
    size_t counts; // Consecutive count capture.
    LAST_SENT lastAlt; // Last alert that was sent.
};

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
    bool monitorSens(SensDownPkg &pkg, size_t errCt);
};

}

#endif // ALERT_HPP