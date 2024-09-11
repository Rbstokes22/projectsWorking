#include "Network/Socket.hpp"
#include "lwip/sockets.h"
#include "Network/NetMain.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include <unistd.h>
#include "string.h"
#include "arpa/inet.h"
#include "sys/select.h"
#include "Config/config.hpp"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"

namespace Comms {

// STATIC
SktFlags SocketServer::flags{false};
int SocketServer::port{DEF_PORT}; // Default
int SocketServer::servSock{-1};
Client SocketServer::clientSocks[MAX_CLIENTS]{0};

SocketServer::SocketServer(
    int port, 
    NetMain &sta, 
    NetMain &ap,
    Threads::Thread &thrd,
    Threads::socketThreadParams &params) : 

    sta(sta), ap(ap), thrd(thrd), params(params) {

        SocketServer::port = port;
    }

SocketServer::~SocketServer() {
    this->stop();
}

// Create task
void SocketServer::start() { 
    if (!SocketServer::flags.taskStarted) {
        printf("Creating Socket Task\n");
        this->thrd.initThread(
            SocketServer::socketTask, 4096, &this->params, 2
            );

        SocketServer::flags.taskStarted = true;
    }
}

void SocketServer::stop() {
    
    if (SocketServer::servSock != -1) {
        close(SocketServer::servSock);
    }

    SocketServer::flags.taskStarted = false;
    this->thrd.~Thread(); // Delete thread
}

// Bind and listen 
void SocketServer::socketTask(void *parameter) {
    Threads::socketThreadParams* params = 
        static_cast<Threads::socketThreadParams*>(parameter);

    int clientSock{0};
    sockaddr_in6 destAddr;
    socklen_t addrLen = sizeof(destAddr);
   
    // data structure used by select to keep track of file descriptors, sockets
    // in this case, that we want to monitor for activity.
    fd_set readfds; 

    // Create Socket
    SocketServer::createSocket(destAddr);
    
    while (true) {
        SocketServer::FDSset(readfds);
    
        // Checks if a specific file descriptor is part of the fd_set.
        // After select returns, this function determines the sockets
        // avaialable for reading.
        if (FD_ISSET(SocketServer::servSock, &readfds)) {

            // Listens for new clients, once it accepts, it passes the client
            // socket ID to the handleClient method.
            clientSock = accept(
                SocketServer::servSock, 
                (struct sockaddr *)&destAddr, 
                &addrLen
                );

            if (clientSock >= 0) {
                printf("Accepted Client\n"); // Delete after testing that it works
                SocketServer::addClients(clientSock);
            } else {
                perror("accept failed");
            }
        } 

        SocketServer::checkActivity(readfds, params->mutex);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    close(SocketServer::servSock);
}

void SocketServer::createSocket(sockaddr_in6 &destAddr) {
    SocketServer::servSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_IP);

    if (SocketServer::servSock < 0) {
        printf("Unable to Create socket\n"); 
        SocketServer::flags.taskStarted = false;
        vTaskDelete(NULL);
    } 

    printf("Created Socket\n");

    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_addr = in6addr_any; // listen on all interfaces
    destAddr.sin6_port = htons(SocketServer::port);

    if (bind(
        SocketServer::servSock, 
        (struct sockaddr*)&destAddr, sizeof(destAddr)) < 0) {

        printf("SocketServer, unable to bind %d\n", errno);
        close(SocketServer::servSock);
        SocketServer::flags.taskStarted = false;
        vTaskDelete(NULL);
    }

    printf("SocketServer, bind complete\n");

    if (listen(SocketServer::servSock, MAX_CLIENTS) < 0) {
        printf("SocketServer, unable to accept connection: %d\n", errno);
        close(SocketServer::servSock);
        SocketServer::flags.taskStarted = false;
        vTaskDelete(NULL);
    }

    printf("Socket Listening\n");
}

void SocketServer::FDSset(fd_set &readfds) {
    // Inits the fd_set structure to be empty to ensure it starts with 
    // no descriptors.
    FD_ZERO(&readfds);

    // Adds file descirptor to the fd_set structure. This is called
    // for each socket we want to monitor before calling select.
    FD_SET(SocketServer::servSock, &readfds);

    // Specifies the highest-numbered file descriptor in the fd_set
    // allowing select() to know the range of descriptors to check.
    int max_sd = SocketServer::servSock;

    // Add the client sockets to the fd_set
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (SocketServer::clientSocks[i].sock > 0) {
            FD_SET(SocketServer::clientSocks[i].sock, &readfds);
        }

        if (clientSocks[i].sock > max_sd) {
            max_sd = SocketServer::clientSocks[i].sock;
        }
    }

    // Monitors multiple file descriptors, or sockets in this case.
    // Select can be used to monitor all types of i/o operations
    // in a non-blocking manner, such as sockets, pipes, rx/tx.
    int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if (activity < 0) perror("select error");
}

void SocketServer::addClients(int clientSock) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (SocketServer::clientSocks[i].sock == 0) {
            SocketServer::clientSocks[i].sock = clientSock;
            break;
        }
    }
}

void SocketServer::checkActivity(fd_set &readfds, Threads::Mutex &mutex) {
// RUN IF SOCKET != 0 in order to process this, that should clear things up.
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (FD_ISSET(SocketServer::clientSocks[i].sock, &readfds)) {

            if (SocketServer::clientSocks[i].sock == 0) continue;

            printf("Serving client %d\n", SocketServer::clientSocks[i].sock);

            int len = recv(
                SocketServer::clientSocks[i].sock, 
                SocketServer::clientSocks[i].rx_buffer, 
                sizeof(SocketServer::clientSocks[i].rx_buffer) -1, 0
                );

            if (len > 0) { // Data Received
                if (len < sizeof(SocketServer::clientSocks[i].rx_buffer)) {
                    SocketServer::clientSocks[i].rx_buffer[len] = '\0';
                }
                
                SocketServer::clientSocks[i].retries = 0;
                SocketServer::processRx(SocketServer::clientSocks[i], mutex, len);

            } else if (len == 0) { // Client Disconnected
                SocketServer::clientSocks[i].retries++;

                if (SocketServer::clientSocks[i].retries > SOCKET_TO_RETRIES) {
                    printf("Client %d discon\n", SocketServer::clientSocks[i].sock);
                    close(SocketServer::clientSocks[i].sock);
                    SocketServer::clientSocks[i].sock = 0; // clear socket
                }   

            } else { // len < 0
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    perror("recv failed");
                    close(SocketServer::clientSocks[i].sock);
                    SocketServer::clientSocks[i].sock = 0; // clear socket
                }
            }
        }
    }
}

// This sets a basic response for testing. In the future it will need
// to read the request and send the requested data.
void SocketServer::processRx(Client &client, Threads::Mutex &mutex, size_t len) {

    if (!SocketServer::Handshake(client)) {
        SocketServer::decodeFrame(client, len);
        printf("Buffer: %s\n", client.rx_buffer);

        char* cmdStr = strtok((char*)client.rx_buffer, "/");
        char* supDataStr = strtok(NULL, "/");
        char* reqIDStr = strtok(NULL, "/");

        if (cmdStr != NULL && supDataStr != NULL && reqIDStr != NULL) {
            int cmd = atoi(cmdStr);
            int supData = atoi(supDataStr);
            SocketServer::sendClient(cmd, supData, reqIDStr, client, mutex);
        } 
    }
}

bool SocketServer::Handshake(Client &client) {

    auto computeSHA1 = [](const char* input, uint8_t* output) {
        mbedtls_sha1_context ctx;
        mbedtls_sha1_init(&ctx);
        mbedtls_sha1_starts(&ctx);
        mbedtls_sha1_update(&ctx, (const unsigned char*)input, strlen(input));
        mbedtls_sha1_finish(&ctx, output);
        mbedtls_sha1_free(&ctx);
    };

    // Check for WebSocket upgrade request
    if (strstr((char*)client.rx_buffer, "Upgrade: websocket") != NULL) {
        const char* k = "Sec-WebSocket-Key: ";
        char *key = strstr((char*)client.rx_buffer, k);

        if (key) {
            key += strlen(k);
            char* end = strstr(key, "\r\n");
            if (end) *end = '\0'; // Null-terminate the key

            // Generate the Sec-WebSocket-Accept response
            const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // WebSocket GUID
            char acceptKey[256]{0};
            snprintf(acceptKey, sizeof(acceptKey), "%s%s", key, guid); // Concatenate key and GUID
            
            // Hash the accept key
            uint8_t sha1Hash[20]{0};

            computeSHA1(acceptKey, sha1Hash);

            // Base64 encode the SHA-1 hash
            size_t outputLength;
            size_t inputSize = sizeof(sha1Hash);
            size_t outputSize = 4 * ((inputSize + 2) / 3) + 1;
            char base64AcceptKey[outputSize]{0};

            int encode = mbedtls_base64_encode(
                (unsigned char*)base64AcceptKey, 
                outputSize, 
                &outputLength, 
                sha1Hash, 
                inputSize
                );

            if (encode != 0) {
                printf("Error with encoded, value is %d\n", encode);
                return false;
            }

            base64AcceptKey[outputLength] = '\0';
            
            char resp[256]{0};

            // Send the WebSocket handshake response
            // Create the WebSocket handshake response
            snprintf(resp, sizeof(resp),
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: %s\r\n"
                        "\r\n", base64AcceptKey);

            printf("resp: %s\n", resp);
            send(client.sock, resp, strlen(resp), 0); // Send the complete response

            return true; // Handshake successful
        }
    }

    return false; // Handshake failed
}

void SocketServer::decodeFrame(Client &client, size_t len) {
    uint8_t* frame = client.rx_buffer;
    uint8_t opcode = frame[0] & 0x0F; // last 4 bits of first byte
    int adjustment{0}; // Used to adjust if extra payload > 126.

    // Payload length. This masks the first bit of the 2nd byte. This byte
    // indicates whether the payload itself is masked or not and needs 
    // to be unmasked by the server, which is 1. Prevents certain type
    // of attacks.
    uint8_t payloadLen = frame[1] & 0x7F; 

    // If 126, it indicates that the actual payload len is in the next
    // 2 bytes. Both byte 3 and 4 are combined into a 16 bit value. 
    // byte 3 is placed in the higher byte of the value, and byte 4
    // is placed into the lower. 127 is rarely used because it indicates
    // a payload of over 65535 chars.
    if (payloadLen == 126) {
        payloadLen = (frame[2] << 8) | frame[3];
        adjustment = 2;
    } 

    // Extract masking key if payload is masked by isolating the MSB.
    uint8_t maskingKey[4]{0};
    if (frame[1] & 0x80) { // check if mask bit is in set
        maskingKey[0] = frame[2 + adjustment];
        maskingKey[1] = frame[3 + adjustment];
        maskingKey[2] = frame[4 + adjustment];
        maskingKey[3] = frame[5 + adjustment];
    }

    // Extract the payload data which begins at byte 9.
    uint8_t* payloadData = &frame[6 + adjustment]; // start of PL data

    // If masked, XOR the payload data with the masking key. 
    for (size_t i = 0; i < payloadLen; ++i) {
        payloadData[i] ^= maskingKey[i % 4]; 
    }

    // Now payload Data contains original message, process based on opcode
    if (opcode == 0x1) { // Text Frame
        memcpy(client.rx_buffer, payloadData, payloadLen);
        client.rx_buffer[payloadLen] = '\0';

    } else if (opcode == 0x2) { // Binary frame
        // Handle binary here

    } else if (opcode == 0x8) { // Close frame request
        printf("Disconnecting Client %d\n", client.sock);
        uint8_t closeFrame[2]{0x88, 0x00};
        send(client.sock, closeFrame, sizeof(closeFrame), 0);
        close(client.sock);
    }
}

void SocketServer::encodeFrame(
    uint8_t* buffer, 
    const char* msg, 
    size_t msgLen, 
    uint8_t opCode,
    size_t &frameLen
    ) {

    int adjustment{0}; // Used to adjust if extra payload > 126.

    // Set FIN bit and opcode.
    buffer[0] = 0x80 | opCode;

    if (msgLen < 126) {
        buffer[1] = msgLen;
        frameLen = 2 + msgLen; // 2 bytes for header
    } else if (msgLen == 126) {
        buffer[1] = 126; // indicate extended length
        buffer[2] = (msgLen >> 8) & 0xFF; // High byte
        buffer[3] = msgLen & 0xFF; // Low byte
        adjustment = 2;
        frameLen = 4 + msgLen; // 4 bytes for header
    } 

    memcpy(&buffer[2 + adjustment], msg, msgLen);
}


int rando() {
    return (rand() % 60) + 60;
}

void SocketServer::sendClient(
    int cmd, int supData, char* id, 
    Client &client, Threads::Mutex &mutex
    ) { 

    char response[TX_BUF_SIZE]{0};
    uint8_t buffer[TX_BUF_SIZE + 6]{0}; // + 6 accounts for frame bytes
    size_t frameLen{0};

    switch (static_cast<CMDS>(cmd)) {
        case CMDS::GET_VERSION:
        snprintf(response, sizeof(response), "%s/%s", FIRMWARE_VERSION, id);
        break;

        case CMDS::GET_TEMP:
        snprintf(response, sizeof(response), "%d/%s", rando(), id);
        break;

        case CMDS::GET_HUM:
        snprintf(response, sizeof(response), "%d/%s", rando(), id);
        break;

        default:
        return;
    }

    SocketServer::encodeFrame(
        buffer, response, strlen(response), 
        0x01, frameLen
        );

    printf("Frame len: %d\n", frameLen);

    int bytesSent = send(client.sock, buffer, frameLen, 0);
    if (bytesSent < 0) perror("send failed");
}

}