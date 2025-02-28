#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"
#include "Config/config.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"

namespace Comms {

// Static Setup
const char* SOCKHAND::tag("(SOCKHAND)");
char SOCKHAND::log[LOG_MAX_ENTRY]{0};
Peripheral::Relay* SOCKHAND::Relays{nullptr};
bool SOCKHAND::isInit{false};
argPool SOCKHAND::pool;

// Serves as a pool of resp_arg objects in order to prevent the requirement
// to dynamically allocate memory and keep everything on the stack. The pool
// is quite large considering the size of the buffer inside.
argPool::argPool() : pool() {memset(this->inUse, false, SKT_MAX_RESP_ARGS);}

// Requires no params. Iterates the arg pool and returns a pointer to an open
// allocation. Returns NULL if no availabilities.
async_resp_arg* argPool::getArg() {

    // Iterate Pool, and if in use is false, changes flag to true, zeros it
    // out, and returns a pointer to the pool.
    for (int i = 0; i < SKT_MAX_RESP_ARGS; i++) {
        if (!this->inUse[i]) {
            this->inUse[i] = true;
            memset(&this->pool[i], 0, sizeof(this->pool[i]));
            return &this->pool[i];
        }
    }

    return NULL; // No avail.
}

// Requires the argument pointer. Iterates pool to check the address of the
// argument and the pool argument. If a match, changes the inuse flag to false
// to allow re-allocation.
void argPool::releaseArg(async_resp_arg* arg) {

    // Iterates and changes flag to false if passed arg matches a pool arg.
    for (int i = 0; i < SKT_MAX_RESP_ARGS; i++) {
        if (&this->pool[i] == arg) {
            this->inUse[i] = false;
            break;
        }
    }
}

// Requires the handle, request, and payload buffer. Parses request 
// for an int command, int supplementary info, and char response id.
// Once parsed, returns httpd_queue_work to put the response in the 
// async queue. Returns ESP_FAIL if the data is improperly passed to
// socket server or there are no available structs.
esp_err_t SOCKHAND::trigger_async_send(httpd_handle_t handle, httpd_req_t* req,
    async_resp_arg* arg) {

    // parse/tokenize the data for the cmd, supp, and id. Must be ran 
    // sequentially. cmd is the command, supp is supplementary data such 
    // as temperature value, and id is the message ID sent by the client to
    // ensure the async response is to the requested message.
    const char* cmd = strtok((char*)arg->Buf, "/");
    const char* supp = strtok(NULL, "/");
    const char* id = strtok(NULL, "/");

    if (cmd == NULL || supp == NULL || id == NULL) {
        snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
            "%s Invalid command input format", SOCKHAND::tag);
        
        SOCKHAND::sendErr(SOCKHAND::log);
        return ESP_FAIL;
    }

    // Set up the arg struct with the required data for the queue.
    arg->hd = req->handle; // hd = handle
    arg->fd = httpd_req_to_sockfd(req); // fd = file desriptor
    arg->data.cmd = static_cast<CMDS>(atoi(cmd)); // Convert str to int
    arg->data.suppData = atoi(supp); // Convert to str to int
    strncpy(arg->data.idNum, id, sizeof(arg->data.idNum)); // Copy id.
    arg->data.idNum[sizeof(arg->data.idNum)] = '\0'; // ensures null term
    
    return httpd_queue_work(handle, SOCKHAND::ws_async_send, arg);
}

// Requires arg which will be cast to async_resp_arg. Once available, uses
// the command to determine the request type, uses the supplementary data to
// make changes, and uses the id to return the id with the requested client 
// data, allowing the client to associate response with request ID.
void SOCKHAND::ws_async_send(void* arg) {
    httpd_ws_frame_t wsPkt; // web socket packet.
    struct async_resp_arg* respArg = static_cast<async_resp_arg*>(arg);
    size_t bufSize = sizeof(respArg->Buf);

    memset(respArg->Buf, 0, bufSize); // zeros out.

    // compiles data by executing commands and populating the buffer with the
    // reply. This is the meat and potatoes of fulfilling the request and 
    // sending the reply.
    SOCKHAND::compileData(respArg->data, (char*)respArg->Buf, bufSize);

    // Sets socket pakcet payload to buffer, to send back to client.
    wsPkt.payload = respArg->Buf;
    wsPkt.len = strlen((char*)respArg->Buf); // strlen of compiled data.
    wsPkt.type = HTTPD_WS_TYPE_TEXT; // Ensures text type is sent back to client

    // Async sends response once avaialable using the handle, file descriptor,
    // and the web socket packet.
    esp_err_t asyncSend = httpd_ws_send_frame_async(respArg->hd, respArg->fd, 
        &wsPkt);

    if (asyncSend != ESP_OK) {
        snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
            "%s Skt err sending async response", SOCKHAND::tag);
        
        SOCKHAND::sendErr(SOCKHAND::log);
    }

    // printf("Sending: %s\n", wsPkt.payload); // Used for troubleshooting.

    SOCKHAND::pool.releaseArg(respArg); // Release from arg Pool once done.
}

// Requires message pointer, and level. Level is set to ERROR by default. Sends
// to the log and serial, the printed error.
void SOCKHAND::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg, 
        Messaging::Method::SRL_LOG);
}

// Requires the addresses of the SHT, AS7341, and soil sensor mutex, and the
// relays. Returns true if none of those values == nullptr. False if false.

// Requires the pointer to the relays from main.cpp. Returns true if init,
// and false if not.
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
    // compliance of httpd_uri_t requirements. 

    // Upon the handshake from the client, completes the handshake returning
    // ESP_OK.
    if (req->method == HTTP_GET) {
        snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
            "%s Socket handshake complete", SOCKHAND::tag);
        
        SOCKHAND::sendErr(SOCKHAND::log);
        return ESP_OK;
    }

    // Gets first open arg from the argPool.
    async_resp_arg* arg = SOCKHAND::pool.getArg();

    if (arg == NULL) { // No availabilities
        snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
            "%s No available pool args", SOCKHAND::tag);
        
        SOCKHAND::sendErr(SOCKHAND::log);
        return ESP_OK; // NOTE 1
    }

    httpd_ws_frame_t wsPkt; // Web socket packet
    memset(&wsPkt, 0, sizeof(httpd_ws_frame_t)); // zero out
    wsPkt.type = HTTPD_WS_TYPE_TEXT; // Set type to text.

    // Receives the frame from the client to determine the length of the 
    // incomming frame. By passing 0, we are asking the function to fill
    // in the wsPkt.len field with the requested size.
    esp_err_t ret = httpd_ws_recv_frame(req, &wsPkt, 0);

    if (ret != ESP_OK) {

        snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
            "%s httpd_ws_rec_frame failed to get frame length. %s", 
            SOCKHAND::tag, esp_err_to_name(ret));
        
        SOCKHAND::sendErr(SOCKHAND::log);
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

            snprintf(SOCKHAND::log, sizeof(SOCKHAND::log),
                "%s httpd_ws_rec_frame failed. %s", 
                SOCKHAND::tag, esp_err_to_name(ret));
            
            SOCKHAND::sendErr(SOCKHAND::log);
            return ESP_OK; // NOTE 1.
        }

        // printf("Received packet with msg: %s\n", wsPkt.payload); // TS
    }

    // printf("Frame length is %d\n", wsPkt.len); // TS

    // If the packet type matches, an async trigger is sent,
    // this is completed by request from the client.
    if (wsPkt.type == HTTPD_WS_TYPE_TEXT) {
        // printf("From Client: %s\n", wsPkt.payload); // TS
        SOCKHAND::trigger_async_send(req->handle, req, arg);
    }

    return ESP_OK;
}

}