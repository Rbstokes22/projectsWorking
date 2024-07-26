#ifndef NETCREDS_HPP
#define NETCREDS_HPP

#include "NVS/NVS.hpp"
#include "Network/NetConfig.hpp"

namespace NVS {

class Creds {
    private:
    Messaging::MsgLogHandler &msglogerr;
    NVSctrl nvs;
    char credData[static_cast<int>(Comms::IDXSIZE::PASS)]; // largest array size

    public:
    Creds(char nameSpace[12], Messaging::MsgLogHandler &msglogerr);
    void write(const char* key, const char* buffer, size_t size);
    const char* read(const char* key);
};

}

#endif // NETCREDS_HPP