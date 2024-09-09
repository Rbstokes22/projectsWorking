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
        vTaskDelay(500 / portTICK_PERIOD_MS);
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
                SocketServer::clientSocks[i].rx_buffer[len] = '\0';
                SocketServer::clientSocks[i].retries = 0;
                SocketServer::processRx(SocketServer::clientSocks[i], mutex);

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
void SocketServer::processRx(Client &client, Threads::Mutex &mutex) {

    if (!SocketServer::checkUpgrade(client)) {
        char* cmdStr = strtok(client.rx_buffer, "/");
        char* supDataStr = strtok(NULL, "/");
        char* reqIDStr = strtok(NULL, "/");

        if (cmdStr != NULL && supDataStr != NULL && reqIDStr != NULL) {
            int cmd = atoi(cmdStr);
            int supData = atoi(supDataStr);
            SocketServer::sendClient(cmd, supData, reqIDStr, client, mutex);
        } 
    }
}

bool SocketServer::checkUpgrade(Client &client) {
    auto computeSHA1 = [](const char* input, uint8_t* output) {
        mbedtls_sha1_context ctx;
        mbedtls_sha1_init(&ctx);
        mbedtls_sha1_starts(&ctx);
        mbedtls_sha1_update(&ctx, (const unsigned char*)input, strlen(input));
        mbedtls_sha1_finish(&ctx, output);
        mbedtls_sha1_free(&ctx);
    };

    // Check for WebSocket upgrade request
    if (strstr(client.rx_buffer, "Upgrade: websocket") != NULL) {
        const char* k = "Sec-WebSocket-Key: ";
        char *key = strstr(client.rx_buffer, k);
        printf("Client sockid: %d, Buffer: %s, Retries: %d\n",
               client.sock, client.rx_buffer, client.retries);
        
        if (key) {
            key += strlen(k);
            char* end = strstr(key, "\r\n");
            if (end) *end = '\0'; // Null-terminate the key

            // Generate the Sec-WebSocket-Accept response
            const char *guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // WebSocket GUID
            char acceptKey[256];
            snprintf(acceptKey, sizeof(acceptKey), "%s%s", key, guid); // Concatenate key and GUID
            
            // Hash the accept key
            uint8_t sha1Hash[20];
            computeSHA1(acceptKey, sha1Hash);

            // Base64 encode the SHA-1 hash
            size_t outputLength;
            char *base64AcceptKey = (char*)malloc(4 * ((sizeof(sha1Hash) + 2) / 3)); // Allocate enough space for Base64 encoding
            if (base64AcceptKey != NULL) {
                if (mbedtls_base64_encode((unsigned char*)base64AcceptKey, 4 * ((sizeof(sha1Hash) + 2) / 3), &outputLength, sha1Hash, sizeof(sha1Hash)) != 0) {
                    free(base64AcceptKey); // Free memory on error
                    return false;
                }

                // Send the WebSocket handshake response
                const char *response = "HTTP/1.1 101 Switching Protocols\r\n"
                                       "Upgrade: websocket\r\n"
                                       "Connection: Upgrade\r\n"
                                       "Sec-WebSocket-Accept: ";
                send(client.sock, response, strlen(response), 0);
                send(client.sock, base64AcceptKey, outputLength, 0); // Send the Base64 encoded key
                send(client.sock, "\r\n", 2, 0); // End of headers

                free(base64AcceptKey); // Free the allocated memory for the base64 key

                return true; // Handshake successful
            }
        }
    }
    return false; // Handshake failed
}

int rando() {
    return (rand() % 60) + 60;
}

void SocketServer::sendClient(
    int cmd, int supData, char* id, 
    Client &client, Threads::Mutex &mutex
    ) { 

    char response[TX_BUF_SIZE]{0};

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

    int bytesSent = send(client.sock, response, strlen(response), 0);
    if (bytesSent < 0) perror("send failed");
}

}