#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"
#include "Config/config.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"

namespace Comms {

// Static Setup
Peripheral::Relay* SOCKHAND::Relays{nullptr};
bool SOCKHAND::isInit{false};
argPool SOCKHAND::pool;

// Serves as a pool of resp_arg objects in order to prevent the requirement
// to dynamically allocate memory and keep everything on the stack.
argPool::argPool() : pool(), inUse{false} {}

// Iterates the arg pool and returns a pointer to an open allocation.
async_resp_arg* argPool::getArg() {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (!this->inUse[i]) {
            this->inUse[i] = true;
            memset(&this->pool[i], 0, sizeof(this->pool[i]));
            return &this->pool[i];
        }
    }
    return NULL;
}

// Iterates arg pool and releases it to be used again.
void argPool::releaseArg(async_resp_arg* arg) {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (&this->pool[i] == arg) {
            this->inUse[i] = false;
            break;
        }
    }
}

// Receives the handle, request, and payload buffer. Parses request 
// for an int command, int supplementary info, and char response id.
// Once parsed, returns httpd_queue_work to put the response in the 
// async queue. Returns ESP_FAIL if the data is improperly passed to
// socket server or there are no available structs.
esp_err_t SOCKHAND::trigger_async_send(
    httpd_handle_t handle, 
    httpd_req_t* req,
    async_resp_arg* arg
    ) {

    // parse the data for the cmd, supp, and id.
    const char* cmd = strtok((char*)arg->Buf, "/");
    const char* supp = strtok(NULL, "/");
    const char* id = strtok(NULL, "/");

    if (cmd == NULL || supp == NULL || id == NULL) {
        printf("Invalid input format\n");
        return ESP_FAIL;
    }

    // Set up the arg struct with the required data for the queue.
    arg->hd = req->handle;
    arg->fd = httpd_req_to_sockfd(req);
    arg->data.cmd = static_cast<CMDS>(atoi(cmd));
    arg->data.suppData = atoi(supp);

    strncpy(arg->data.idNum, id, sizeof(arg->data.idNum));
    arg->data.idNum[sizeof(arg->data.idNum)] = '\0';
    
    return httpd_queue_work(handle, SOCKHAND::ws_async_send, arg);
}

// Once available, receives the argument containing all required data.
// Uses the command to determine request type, uses supplementary info
// to make changes/ and uses the id to return the id with the requested
// data so the client knows how to handle the incoming data.
void SOCKHAND::ws_async_send(void* arg) {
    httpd_ws_frame_t wsPkt;
    struct async_resp_arg* respArg = static_cast<async_resp_arg*>(arg);
    size_t bufSize = sizeof(respArg->Buf);

    memset(respArg->Buf, 0, bufSize);
    SOCKHAND::compileData(respArg->data, (char*)respArg->Buf, bufSize);

    // Sets payload to send back to client.
    wsPkt.payload = respArg->Buf;
    wsPkt.len = strlen((char*)respArg->Buf);
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    // Async sends response.
    httpd_ws_send_frame_async(respArg->hd, respArg->fd, &wsPkt);
    // printf("Sending: %s\n", wsPkt.payload); 

    SOCKHAND::pool.releaseArg(respArg); // Release from arg Pool.
}



// Requires the address of the DHT, AS7341, and soil sensor mutex, and the
// relays. Returns true if none of those values == nullptr. False if false.
bool SOCKHAND::init(Peripheral::Relay* relays) {

        Relays = relays;
        if (relays != nullptr) SOCKHAND::isInit = true;
        return SOCKHAND::isInit;
    }

// Websocket handler receives request. Completes the handshake. Handles
// all follow on socket transmission. Returns ESP_OK ONLY in compliance
// with the route http_uri_t which will close socket if ESP_OK isnt
// returned.
esp_err_t SOCKHAND::wsHandler(httpd_req_t* req) {
    // NOTE 1: Returns ESP_OK to block the rest of the code being ran in 
    // compliance of httpd_uri_t. 

    // Upon the handshake from the client, completes the handshake returning
    // ESP_OK.
    if (req->method == HTTP_GET) {
        printf("Socket Handshake complete\n");
        return ESP_OK;
    }

    // Gets arg from the argPool.
    async_resp_arg* arg = SOCKHAND::pool.getArg();
    if (arg == NULL) {
        printf("No available structures\n");
        return ESP_OK; // NOTE 1
    }

    // Works off a dual buffer system, alternating buffers to allow copying of 
    // data to the async_resp_arg structure, to be handle async.
    // uint8_t* buffer = getBuffer();
    esp_err_t ret;

    httpd_ws_frame_t wsPkt;
    memset(&wsPkt, 0, sizeof(httpd_ws_frame_t));
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    // Receives the frame from the client to determine the length of the 
    // incomming frame. By passing 0, we are asking the function to fill
    // in the wsPkt.len field with the requested size.
    ret = httpd_ws_recv_frame(req, &wsPkt, 0);
    if (ret != ESP_OK) {
        printf("httpd_ws_rec_frame failed to get frame len with %d\n", ret);
        return ESP_OK; // NOTE 1
    }

    if (wsPkt.len) {
        // Sets the payload to the pre-allocated buffer.
        wsPkt.payload = arg->Buf;

        // Calling this again, we are asking the function to fill in the
        // wsPkt.payload with the clients payload using the length of the
        // packets.
        ret = httpd_ws_recv_frame(req, &wsPkt, wsPkt.len);
        if (ret != ESP_OK) {
            printf("httpd_ws_rec_frame failed with %d\n", ret);
            return ESP_OK; // NOTE 1.
        }

        // printf("Received packet with msg: %s\n", wsPkt.payload);
    }

    // printf("Frame length is %d\n", wsPkt.len);

    // If the packet type matches, an async trigger is sent.
    if (wsPkt.type == HTTPD_WS_TYPE_TEXT) {
        // printf("From Client: %s\n", wsPkt.payload);
        SOCKHAND::trigger_async_send(req->handle, req, arg);
    }

    return ESP_OK;
}





}