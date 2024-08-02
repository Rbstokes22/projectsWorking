#ifndef WAPSETUP_HPP
#define WAPSETUP_HPP

#include "esp_http_server.h"
#include <cstddef>
#include "Network/NetSTA.hpp"
#include "Network/NetWAP.hpp"
#include "Network/NetCreds.hpp"

namespace Comms {

void setJSONObjects(NetSTA &_station, NetWAP &_wap, NVS::Creds &_creds);
esp_err_t WAPSetupIndexHandler(httpd_req_t *req);
esp_err_t WAPSubmitDataHandler(httpd_req_t *req);

}

#endif // WAPSETUP_HPP