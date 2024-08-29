#ifndef WAPSETUPHANDLER_HPP
#define WAPSETUPHANDLER_HPP

#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"
#include "cJSON.h"

namespace Comms {

void setJSONObjects(NetSTA &_station, NetWAP &_wap, NVS::Creds &_creds);
cJSON* receiveJSON(httpd_req_t* req);
esp_err_t processJSON(cJSON* json, httpd_req_t* req, char* writtenKey);
esp_err_t respondJSON(httpd_req_t* req, char* writtenKey);
esp_err_t WAPSetupIndexHandler(httpd_req_t* req);
esp_err_t WAPSubmitDataHandler(httpd_req_t* req);

}

#endif // WAPSETUPHANDLER_HPP