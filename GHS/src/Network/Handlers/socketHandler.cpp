#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"
#include "Config/config.hpp"

namespace Comms {

// Serves as a pool of resp_arg objects in order to prevent the requirement
// to dynamically allocate memory and keep everything on the stack.
struct async_resp_arg argPool[MAX_RESP_ARGS] = {0};
bool inUse[MAX_RESP_ARGS] = {false};

// Iterates the arg pool and returns a pointer to an open allocation.
struct async_resp_arg* getArg() {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (!inUse[i]) {
            inUse[i] = true;
            memset(&argPool[i], 0, sizeof(argPool[i]));
            return &argPool[i];
        }
    }
    return NULL;
}

// Iterates arg pool and releases it to be used again.
void releaseArg(struct async_resp_arg* arg) {
    for (int i = 0; i < MAX_RESP_ARGS; i++) {
        if (&argPool[i] == arg) {
            inUse[i] = false;
            break;
        }
    }
}

// Websocket handler receives request. Completes the handshake. Handles
// all follow on socket transmission. Returns ESP_OK, ESP_FAIL, 
// ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_ARG and if all successful,
// returns trigger_async_send().
esp_err_t wsHandler(httpd_req_t* req) {
    
    // Upon the handshake from the client, completes the handshake returning
    // ESP_OK.
    if (req->method == HTTP_GET) {
        printf("Socket Handshake complete\n");
        return ESP_OK;
    }

    // Gets arg from the argPool.
    struct async_resp_arg* arg = getArg();
    if (arg == NULL) {
        printf("No avaialable structures\n");
        return ESP_FAIL;
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
        return ret;
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
            return ret;
        }

        // printf("Received packet with msg: %s\n", wsPkt.payload);
    }

    // printf("Frame length is %d\n", wsPkt.len);

    // If the packet type matches, an async trigger is sent.
    if (wsPkt.type == HTTPD_WS_TYPE_TEXT) {
        // printf("From Client: %s\n", wsPkt.payload);
        return trigger_async_send(req->handle, req, arg);
    }

    return ESP_OK;
}

// Receives the handle, request, and payload buffer. Parses request 
// for an int command, int supplementary info, and char response id.
// Once parsed, returns httpd_queue_work to put the response in the 
// async queue. Returns ESP_FAIL if the data is improperly passed to
// socket server or there are no available structs.
esp_err_t trigger_async_send(
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
    
    return httpd_queue_work(handle, ws_async_send, arg);
}

// Once available, receives the argument containing all required data.
// Uses the command to determine request type, uses supplementary info
// to make changes/ and uses the id to return the id with the requested
// data so the client knows how to handle the incoming data.
void ws_async_send(void* arg) {
    httpd_ws_frame_t wsPkt;
    struct async_resp_arg* respArg = static_cast<async_resp_arg*>(arg);
    size_t bufSize = sizeof(respArg->Buf);

    memset(respArg->Buf, 0, bufSize);
    compileData(respArg->data, (char*)respArg->Buf, bufSize);

    // Sets payload to send back to client.
    wsPkt.payload = respArg->Buf;
    wsPkt.len = strlen((char*)respArg->Buf);
    wsPkt.type = HTTPD_WS_TYPE_TEXT;

    // Async sends response.
    httpd_ws_send_frame_async(respArg->hd, respArg->fd, &wsPkt);
    // printf("Sending: %s\n", wsPkt.payload); 

    releaseArg(respArg); // Release from arg Pool.
}

int rando() {
    return rand() % 101;
}

void compileData(cmdData &data, char* buffer, size_t size) {
    int written{0};
    // All commands work from here.
    switch(data.cmd) {
        case CMDS::GET_ALL:
        written = snprintf(buffer, size, 
        "{\"firmv\":\"%s\",\"temp\":%d,\"hum\":%d,\"id\":\"%s\"}", 
        FIRMWARE_VERSION, 
        rando(), 
        rando(),
        data.idNum
        );

        break;

        case CMDS::SET_MIN_TEMP:
        break;

        case CMDS::SET_MAX_TEMP:
        break;

        case CMDS::SET_MIN_HUM:
        break;

        case CMDS::SET_MAX_HUM:
        break;
    }

    if (written <= 0) {
        printf("Error formatting string\n");
    } else if (written >= size) {
        printf("Output was truncated. Buffer size: %zu, Output size: %d\n",
        size, written);
    } 
}

}