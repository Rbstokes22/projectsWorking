#ifndef WAPSETUPHANDLER_HPP
#define WAPSETUPHANDLER_HPP

#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "cJSON.h"
#include "UI/MsgLogHandler.hpp"
#include "Network/Handlers/MasterHandler.hpp"

namespace Comms {

#define WAPHANDLER_JSON_SIZE 128 // buffer bytes.

class WSHAND : public MASTERHAND { // WAP SETUP HANDLER
    private:
    static const char* tag;
    static char log[LOG_MAX_ENTRY]; // Used to log errors and messages
    static NetSTA* station; // STA ptr. Used to copy creds to class instance.
    static NetWAP* wap; // WAP ptr. Used to copy creds to class instance.
    static bool isInit; // Is instance initialized.
    static cJSON* receiveJSON(httpd_req_t* req);
    static esp_err_t processJSON(cJSON* json, httpd_req_t* req, 
        char* writtenKey);

    static esp_err_t respondJSON(httpd_req_t* req, char* writtenKey);
    static bool initCheck(httpd_req_t* req);
    static bool sendstrErr(esp_err_t err, const char* src);
    static void sendErr(const char* msg, 
        Messaging::Levels lvl = Messaging::Levels::ERROR);
    
    public:
    static bool init(NetSTA &station, NetWAP &wap);
    static esp_err_t IndexHandler(httpd_req_t* req);
    static esp_err_t DataHandler(httpd_req_t* req);
};

}

#endif // WAPSETUPHANDLER_HPP