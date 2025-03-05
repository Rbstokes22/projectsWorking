#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"
#include "Config/config.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Relay.hpp"
#include "UI/MsgLogHandler.hpp"

namespace Comms {

// Static Setup
const char* SOCKHAND::tag("(SOCKHAND)");
Peripheral::Relay* SOCKHAND::Relays{nullptr};
bool SOCKHAND::isInit{false};
argPool SOCKHAND::pool;

// Serves as a pool of resp_arg objects in order to prevent the requirement
// to dynamically allocate memory and keep everything on the stack. The pool
// is quite large considering the size of the buffer inside.
argPool::argPool() : pool() {
    memset(this->inUse, false, SKT_MAX_RESP_ARGS);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        "(ARGPOOL) Ob created", Messaging::Method::SRL_LOG);
}

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
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Invalid command input format", SOCKHAND::tag);
        
        MASTERHAND::sendErr(MASTERHAND::log);
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
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s Skt err sending async response", SOCKHAND::tag);
        
        MASTERHAND::sendErr(MASTERHAND::log);
    }

    // printf("Sending: %s\n", wsPkt.payload); // Used for troubleshooting.

    SOCKHAND::pool.releaseArg(respArg); // Release from arg Pool once done.
}

// Requires the http request pointer. Checks if the class has been init. If 
// not, returns false and a log entry. If true, returns true. Prevents public 
// function from working if not init. 
bool SOCKHAND::initCheck(httpd_req_t* req) {
    if (req == nullptr) return false;

    if (!SOCKHAND::isInit) { // If not init reply to client and log.

        // This is the only time this will send an json response. Must set here
        // since this is a socket and not the normal response type. This
        // indicates an error only before the handshake.
        if (httpd_resp_set_type(req, MHAND_RESP_TYPE_JSON) != ESP_OK) {
            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
                "%s type not set", SOCKHAND::tag);

            MASTERHAND::sendErr(MASTERHAND::log);
            MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_TYPESET_FAIL),
                SOCKHAND::tag, "initChk");
            return false;
        }

        // Type is set, reply with JSON if not init

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Not init", SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

        // Respond to client to satisfy request.
        MASTERHAND::sendstrErr(httpd_resp_sendstr(req, MHAND_NOT_INIT),
            SOCKHAND::tag, "initChk");
    }

    return SOCKHAND::isInit; // true if init.
}

// Requires the pointer to the relays from main.cpp. Returns true if init,
// and false if not.
bool SOCKHAND::init(Peripheral::Relay* relays) {

    SOCKHAND::isInit = (relays != nullptr); // Ensure no nullptr is passed.

    if (isInit) { // init, send INFO log.

        Relays = relays; // Set relays.
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s init", 
            SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

    } else { // Not init, send CRITICAL log.
        
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), "%s not init", 
            SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::CRITICAL);
    }
    
    return SOCKHAND::isInit;
}

// Websocket handler receives request. Completes the handshake. Handles
// all follow on socket transmission. Returns ESP_OK ONLY in compliance
// with the route http_uri_t which will close socket if ESP_OK isnt
// returned. ATTENTION: This is the entry into this class instance, which is
// why the init check is here. No other functions will be accessed without
// going through this gate.
esp_err_t SOCKHAND::wsHandler(httpd_req_t* req) {
    // NOTE 1: Returns ESP_OK to block the rest of the code being ran in 
    // compliance of httpd_uri_t requirements. 

    if (!SOCKHAND::initCheck(req)) return ESP_OK; // Block if not init.

    // Upon the handshake from the client, completes the handshake returning
    // ESP_OK. This is EXCLUSIVE TO THE HANDSHAKE AND WILL HAPPEN ONCE.
    if (req->method == HTTP_GET) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s handshake complete", SOCKHAND::tag);
        
        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);
        return ESP_OK;
    }

    // HANDSHAKE COMPLETE. Code block above will not run in follow on requests.
    // Gets first open arg from the argPool. 
    async_resp_arg* arg = SOCKHAND::pool.getArg();

    if (arg == NULL) { // No availabilities

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s No available pool args", SOCKHAND::tag);
        
        MASTERHAND::sendErr(MASTERHAND::log);
        return ESP_OK; // NOTE 1. BLOCK.
    }

    // Handshake is complete, and argument is ready for use.

    httpd_ws_frame_t wsPkt; // Web socket packet
    memset(&wsPkt, 0, sizeof(httpd_ws_frame_t)); // zero out
    wsPkt.type = HTTPD_WS_TYPE_TEXT; // Set type to text.

    // Receives the frame from the client to determine the length of the 
    // incomming frame. By passing 0, we are asking the function to fill
    // in the wsPkt.len field with the requested size.
    esp_err_t ret = httpd_ws_recv_frame(req, &wsPkt, 0);

    if (ret != ESP_OK) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
            "%s httpd_ws_rec_frame failed to get frame length. %s", 
            SOCKHAND::tag, esp_err_to_name(ret));
        
        MASTERHAND::sendErr(MASTERHAND::log);

        return ESP_OK; // NOTE 1. BLOCK.
    }

    if (wsPkt.len) { // It exists.

        // Sets the payload to the pre-allocated buffer.
        wsPkt.payload = arg->Buf;

        // Calling this again, we are asking the function to fill in the
        // wsPkt.payload with the clients payload using the length of the
        // packets.
        ret = httpd_ws_recv_frame(req, &wsPkt, wsPkt.len);

        if (ret != ESP_OK) {

            snprintf(MASTERHAND::log, sizeof(MASTERHAND::log),
                "%s httpd_ws_rec_frame failed. %s", 
                SOCKHAND::tag, esp_err_to_name(ret));
            
            MASTERHAND::sendErr(MASTERHAND::log);
            return ESP_OK; // NOTE 1. BLOCK.
        }

        // printf("Received packet with msg: %s\n", wsPkt.payload); // TS
    }

    // printf("Frame length is %d\n", wsPkt.len); // TS

    // If the packet type matches, an async trigger is sent, this is completed
    // by request from the client. A
    if (wsPkt.type == HTTPD_WS_TYPE_TEXT) {
        // printf("From Client: %s\n", wsPkt.payload); // TS
        SOCKHAND::trigger_async_send(req->handle, req, arg);
    }

    return ESP_OK;
}

}