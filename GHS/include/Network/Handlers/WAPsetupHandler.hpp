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
#define WAPHANDLER_WRITTEN_KEY "NULL" // Used to ensure key has been written
#define WAPHANDLER_WRITTEN_KEY_SIZE 16 // Used to 

class WSHAND : public MASTERHAND { // WAP SETUP HANDLER
    private:
    static const char* tag;
    static NetSTA* station; // STA ptr. Used to copy creds to class instance.
    static NetWAP* wap; // WAP ptr. Used to copy creds to class instance.
    static bool isInit; // Is instance initialized.
    static cJSON* receiveJSON(httpd_req_t* req);
    static bool processJSON(cJSON* json, httpd_req_t* req, 
        char* writtenKey);

    static bool respondJSON(httpd_req_t* req, char* writtenKey);
    static bool initCheck(httpd_req_t* req);
    static bool handleNVS(bool write, const char* key, cJSON* item,
        httpd_req_t* req, char* writtenKey);

    public:
    static bool init(NetSTA &station, NetWAP &wap);
    static esp_err_t IndexHandler(httpd_req_t* req); // Entrance
    static esp_err_t DataHandler(httpd_req_t* req); // Entrance
};

}

#endif // WAPSETUPHANDLER_HPP