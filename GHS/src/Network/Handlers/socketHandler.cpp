#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"

namespace Comms {

NetSTA *sta{nullptr};

struct async_resp_arg argPool[MAX_RESP_ARGS] = {0};
bool inUse[MAX_RESP_ARGS] = {false};

void setNetObjs(NetSTA &_sta) {
    sta = &_sta;
}

struct async_resp_arg* getArg() {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (!inUse[i]) {
            inUse[i] = true;
            return &argPool[i];
        }
    }
    return NULL;
}

void releaseArg(struct async_resp_arg* arg) {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (&argPool[i] == arg) {
            inUse[i] = false;
            break;
        }
    }
}

esp_err_t wsHandler(httpd_req_t* req) {
    
    if (req->method == HTTP_GET) {
        printf("Socket Handshake complete\n");
        return ESP_OK;
    }

    uint8_t buffer[BUF_SIZE]{0};
    esp_err_t ret;

    httpd_ws_frame_t wsPkt;
    memset(&wsPkt, 0, sizeof(httpd_ws_frame_t));
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    ret = httpd_ws_recv_frame(req, &wsPkt, 0);
    if (ret != ESP_OK) {
        printf("httpd_ws_rec_frame failed to get frame len with %d\n", ret);
        return ret;
    }

    if (wsPkt.len) {
        wsPkt.payload = buffer;

        ret = httpd_ws_recv_frame(req, &wsPkt, wsPkt.len);
        if (ret != ESP_OK) {
            printf("httpd_ws_rec_frame failed with %d\n", ret);
            return ret;
        }

        printf("Received packet with msg: %s\n", wsPkt.payload);
    }

    printf("Frame length is %d\n", wsPkt.len);

    if (wsPkt.type == HTTPD_WS_TYPE_TEXT) {
        // This is where Command can be done, and then can trigger
        // the async when complete with a message.
        printf("From Client: %s\n", wsPkt.payload);
        return trigger_async_send(req->handle, req);
    }

    return ESP_OK;
}

esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t* req) {

    struct async_resp_arg* arg = getArg();
    if (arg == NULL) {
        printf("No avaialable structures\n");
        return ESP_FAIL;
    }

    arg->hd = req->handle;
    arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, arg);
}

void ws_async_send(void* arg) {
    httpd_ws_frame_t wsPkt;
    struct async_resp_arg* respArg = static_cast<async_resp_arg*>(arg);
    httpd_handle_t hd = respArg->hd;
    int fd = respArg->fd;

    char buffer[128] = "Received from server";
    wsPkt.payload = (uint8_t*)buffer;
    wsPkt.len = strlen(buffer);
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    static size_t maxClients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t fds = maxClients;
    int clientFDS[maxClients];

    httpd_handle_t server = sta->getServer();
    esp_err_t ret = httpd_get_client_list(server, &fds, clientFDS);

    if (ret != ESP_OK) {
        return;
    }

    for (int i = 0; i < fds; i++) {
        int client_info = httpd_ws_get_fd_info(server, clientFDS[i]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(hd, clientFDS[i], &wsPkt);
        }
    }

    releaseArg(respArg);

}

}