#include "Network/Handlers/socketHandler.hpp"
#include "string.h"
#include "Network/NetSTA.hpp"
#include "Config/config.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Relay.hpp"

namespace Comms {

// Static Setup
Threads::Mutex* SOCKHAND::DHTmtx{nullptr};
Threads::Mutex* SOCKHAND::AS7341mtx{nullptr};
Threads::Mutex* SOCKHAND::SOILmtx{nullptr};
Peripheral::Relay* SOCKHAND::Relays{nullptr};
bool SOCKHAND::isInit{false};
argPool SOCKHAND::pool;

// Serves as a pool of resp_arg objects in order to prevent the requirement
// to dynamically allocate memory and keep everything on the stack.
argPool::argPool() : pool{0}, inUse{false} {}

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

void SOCKHAND::compileData(cmdData &data, char* buffer, size_t size) {
    int written{0};

    auto attachRelay = [](int relayNum, Peripheral::BOUNDARY_CONFIG &conf){
        if (relayNum >= 0 && relayNum < 4) {
            uint16_t IDTemp = SOCKHAND::Relays[relayNum].getID();
            conf.relay = &SOCKHAND::Relays[relayNum];
            conf.relayControlID = IDTemp;
            conf.relayNum = relayNum + 1;
        } else if (relayNum == 4) {
            conf.relay = nullptr;
            conf.relayNum = 0;
        } 
    };

    // All commands work from here.
    switch(data.cmd) {
        case CMDS::GET_ALL:
        SOCKHAND::DHTmtx->lock();

        written = snprintf(buffer, size,  
        "{\"firmv\":\"%s\",\"id\":\"%s\",\"re1\":%d,\"re2\":%d,\"re3\":%d,\
        \"re4\":%d,\"temp\":%.2f,\"hum\":%.2f,\"tempRelay\":%d,\"tempCond\"\
        :%u,\"tempRelayVal\":%d,\"tempAlertVal\":%d,\"tempAlertEn\":%d,\
        \"humRelay\":%d,\"humCond\":%u,\"humRelayVal\":%d,\"humAlertVal\":%d,\
        \"humAlertEn\":%d}",
        FIRMWARE_VERSION, 
        data.idNum,
        SOCKHAND::Relays[0].isOn(),
        SOCKHAND::Relays[1].isOn(),
        SOCKHAND::Relays[2].isOn(),
        SOCKHAND::Relays[3].isOn(),
        Peripheral::TempHum::getTemp(),
        Peripheral::TempHum::getHum(),
        Peripheral::TempHum::tempConf.relayNum,
        static_cast<uint8_t>(Peripheral::TempHum::tempConf.condition),
        Peripheral::TempHum::tempConf.tripValRelay,
        Peripheral::TempHum::tempConf.tripValAlert,
        Peripheral::TempHum::tempConf.alertsEn,
        Peripheral::TempHum::humConf.relayNum,
        static_cast<uint8_t>(Peripheral::TempHum::humConf.condition),
        Peripheral::TempHum::humConf.tripValRelay,
        Peripheral::TempHum::humConf.tripValAlert,
        Peripheral::TempHum::humConf.alertsEn
        );

        SOCKHAND::DHTmtx->unlock();

        break;

        case CMDS::RELAY_1:
        static uint16_t IDR1 = SOCKHAND::Relays[0].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[0].off(IDR1);
        } else {
            SOCKHAND::Relays[0].on(IDR1);
        }
        break;

        case CMDS::RELAY_2:
        static uint16_t IDR2 = SOCKHAND::Relays[1].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[1].off(IDR2);
        } else {
            SOCKHAND::Relays[1].on(IDR2);
        }
        break;

        case CMDS::RELAY_3:
        static uint16_t IDR3 = SOCKHAND::Relays[2].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[2].off(IDR3);
        } else {
            SOCKHAND::Relays[2].on(IDR3);
        }
        break;


        case CMDS::RELAY_4:
        static uint16_t IDR4 = SOCKHAND::Relays[3].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[3].off(IDR4);
        } else {
            SOCKHAND::Relays[3].on(IDR4);
        }
        break;

        case CMDS::ATTACH_TEMP_RELAY:
        attachRelay(data.suppData, Peripheral::TempHum::tempConf);
        break;

        case CMDS::SET_TEMP_LWR_THAN:
        Peripheral::TempHum::tempConf.condition = Peripheral::CONDITION::LESS_THAN;
        Peripheral::TempHum::tempConf.tripValRelay = data.suppData;
        break;

        case CMDS::SET_TEMP_GTR_THAN:
        Peripheral::TempHum::tempConf.condition = Peripheral::CONDITION::GTR_THAN;
        Peripheral::TempHum::tempConf.tripValRelay = data.suppData;
        break;

        case CMDS::SET_TEMP_COND_NONE:
        Peripheral::TempHum::tempConf.condition = Peripheral::CONDITION::NONE;
        break;

        case CMDS::ENABLE_TEMP_ALERT:
        Peripheral::TempHum::tempConf.tripValAlert = data.suppData;
        Peripheral::TempHum::tempConf.alertsEn = true;
        break;

        case CMDS::DISABLE_TEMP_ALERT:
        Peripheral::TempHum::tempConf.alertsEn = false;
        break;

        case CMDS::ATTACH_HUM_RELAY:
        attachRelay(data.suppData, Peripheral::TempHum::humConf);
        break;

        case CMDS::SET_HUM_LWR_THAN:
        Peripheral::TempHum::humConf.condition = Peripheral::CONDITION::LESS_THAN;
        Peripheral::TempHum::humConf.tripValRelay = data.suppData;
        break;

        case CMDS::SET_HUM_GTR_THAN:
        Peripheral::TempHum::humConf.condition = Peripheral::CONDITION::GTR_THAN;
        Peripheral::TempHum::humConf.tripValRelay = data.suppData;
        break;

        case CMDS::SET_HUM_COND_NONE:
        Peripheral::TempHum::humConf.condition = Peripheral::CONDITION::NONE;
        break;

        case CMDS::ENABLE_HUM_ALERT:
        Peripheral::TempHum::humConf.tripValAlert = data.suppData;
        Peripheral::TempHum::humConf.alertsEn = true;
        break;

        case CMDS::DISABLE_HUM_ALERT:
        Peripheral::TempHum::humConf.alertsEn = false;
    }

    if (written < 0) {
        printf("Error formatting string\n");
    } else if (written >= size) {
        printf("Output was truncated. Buffer size: %zu, Output size: %d\n",
        size, written);
    } 
}

// Requires the address of the DHT, AS7341, and soil sensor mutex, and the
// relays. Returns true if none of those values == nullptr. False if false.
bool SOCKHAND::init(
    Threads::Mutex &dhtMUTEX, 
    Threads::Mutex &as7341MUTEX, 
    Threads::Mutex &soilMUTEX,
    Peripheral::Relay* relays
    ) {
        DHTmtx = &dhtMUTEX;
        AS7341mtx = &as7341MUTEX;
        SOILmtx = &soilMUTEX;
        Relays = relays;

        if (DHTmtx != nullptr && AS7341mtx != nullptr &&
            SOILmtx != nullptr && relays != nullptr) {

            SOCKHAND::isInit = true;
        }

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