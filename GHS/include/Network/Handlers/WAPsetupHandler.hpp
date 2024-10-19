#ifndef WAPSETUPHANDLER_HPP
#define WAPSETUPHANDLER_HPP

#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "cJSON.h"

namespace Comms {

class WSHAND { // WAP SETUP HANDLER
    private:
    static NetSTA* station;
    static NetWAP* wap;
    static NVS::Creds* creds;
    static bool isInit;
    static cJSON* receiveJSON(httpd_req_t* req);
    static esp_err_t processJSON(cJSON* json, httpd_req_t* req, char* writtenKey);
    static esp_err_t respondJSON(httpd_req_t* req, char* writtenKey);
    
    public:
    static bool init(NetSTA &station, NetWAP &wap, NVS::Creds &creds);
    static esp_err_t IndexHandler(httpd_req_t* req);
    static esp_err_t DataHandler(httpd_req_t* req);
};

}

#endif // WAPSETUPHANDLER_HPP