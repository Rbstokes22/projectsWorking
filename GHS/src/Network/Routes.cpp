#include "Network/Routes.hpp"
#include "esp_http_server.h"
#include "Network/Handlers/WAPsetupHandler.hpp"
#include "Network/Handlers/WAPHandler.hpp"
#include "Network/Handlers/STAHandler.hpp"
#include "Network/Handlers/socketHandler.hpp"

namespace Comms {

// WAP Setup Routes
httpd_uri_t WAPSetupIndex = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = WAPSetupIndexHandler,
    .user_ctx  = NULL
};

httpd_uri_t WAPSubmitCreds = {
    .uri       = "/SubmitCreds",
    .method    = HTTP_POST,
    .handler   = WAPSubmitDataHandler,
    .user_ctx  = NULL
};

// WAP Routes
httpd_uri_t WAPIndex = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = WAPIndexHandler,
    .user_ctx  = NULL
};

// STA Routes
httpd_uri_t STAIndex = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = STAIndexHandler,
    .user_ctx  = NULL
};

httpd_uri_t OTAUpdate = {
    .uri       = "/OTAUpdate",
    .method    = HTTP_GET,
    .handler   = OTAUpdateHandler,
    .user_ctx  = NULL
};

httpd_uri_t OTAUpdateLAN = {
    .uri       = "/OTAUpdateLAN",
    .method    = HTTP_GET,
    .handler   = OTAUpdateLANHandler,
    .user_ctx  = NULL
};

httpd_uri_t OTARollback = {
    .uri       = "/OTARollback",
    .method    = HTTP_GET,
    .handler   = OTARollbackHandler,
    .user_ctx  = NULL
};

httpd_uri_t OTACheck = {
    .uri       = "/OTACheck",
    .method    = HTTP_GET,
    .handler   = OTACheckHandler,
    .user_ctx  = NULL
};

// Station and WAP routes
httpd_uri_t ws = {
    .uri          = "/ws",
    .method       = HTTP_GET,
    .handler      = wsHandler,
    .user_ctx     = NULL,
    .is_websocket = true
};

}