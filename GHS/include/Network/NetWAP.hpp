#ifndef NETWAP_HPP
#define NETWAP_HPP

#include "Network/NetMain.hpp"

namespace Communications {

class NetWAP : public NetMain{
    private:

    public:
    NetWAP(Messaging::MsgLogHandler &msglogerr);
    void init_wifi() override;
    void start_wifi() override;
    void start_server() override;
    void destroy() override;

};
    
}

#endif // NETWAP_HPP