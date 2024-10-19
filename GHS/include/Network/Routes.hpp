#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "esp_http_server.h"
#include "Network/Handlers/WAPsetupHandler.hpp"

namespace Comms {

extern httpd_uri_t WAPSetupIndex;
extern httpd_uri_t WAPSubmitCreds;
extern httpd_uri_t WAPIndex;
extern httpd_uri_t STAIndex;
extern httpd_uri_t OTAUpdate;
extern httpd_uri_t OTAUpdateLAN;
extern httpd_uri_t OTARollback;
extern httpd_uri_t OTACheck;
extern httpd_uri_t ws;

}

#endif // ROUTES_HPP