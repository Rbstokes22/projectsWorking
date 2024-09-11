#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "lwip/sockets.h"
#include "Network/NetMain.hpp"
#include "Threads/Threads.hpp"
#include "Threads/ThreadParameters.hpp"
#include <unistd.h>
#include "string.h"
#include "arpa/inet.h"
#include "sys/select.h"

namespace Comms {

#define MAX_CLIENTS 4
#define DEF_PORT 8080 // Can be overriden upon creation
#define RX_BUF_SIZE 512
#define TX_BUF_SIZE 256
#define SOCKET_TO_RETRIES 2 // Time until socket closes if no resp

struct Client {
    int sock;
    uint8_t rx_buffer[RX_BUF_SIZE];
    size_t retries;
};

struct SktFlags {
    bool taskStarted;
};

enum class CMDS {
    GET_VERSION, GET_TEMP, GET_HUM
};
    
class SocketServer {
    private:
    static int port;
    static int servSock;
    static Client clientSocks[MAX_CLIENTS];
    NetMain &sta;
    NetMain &ap;
    static SktFlags flags;
    Threads::Thread &thrd;
    Threads::socketThreadParams &params;

    public:
    SocketServer(
        int port, 
        NetMain &sta, 
        NetMain &ap, 
        Threads::Thread &thrd,
        Threads::socketThreadParams &params
        );

    ~SocketServer();
    void start();
    void stop();
    static void socketTask(void *pvParameters);
    static void createSocket(sockaddr_in6 &destAddr);
    static void FDSset(fd_set &readfds);
    static void addClients(int clientSock);
    static void checkActivity(fd_set &readfds, Threads::Mutex &mutex);
    static void processRx(Client &client, Threads::Mutex & mutex, size_t len);
    static bool Handshake(Client &client);
    static void decodeFrame(Client &client, size_t len);
    static void encodeFrame(
        uint8_t* buffer, 
        const char* msg, 
        size_t msgLen, 
        uint8_t opCode,
        size_t &frameLen
        );

    static void sendClient(
        int cmd, int supData, char* id, 
        Client &client, Threads::Mutex & mutex
        );

};

}

#endif // SOCKET_HPP