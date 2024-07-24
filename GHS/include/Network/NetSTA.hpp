#ifndef NETSTA_HPP
#define NETSTA_HPP

#include "Network/NetMain.hpp"

namespace Communications {

class NetSTA : public NetMain {
    private:

    public:
    NetSTA(Messaging::MsgLogHandler &msglogerr);
    void init_wifi() override;
    void start_wifi() override;
    void start_server() override;
    void destroy() override;

};

}

#endif // NETSTA_HPP