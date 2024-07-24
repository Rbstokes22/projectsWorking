#include "Network/NetMain.hpp"

namespace Communications {

// Static Setup
NetMode NetMain::NetType{NetMode::NONE};

NetMain::NetMain(Messaging::MsgLogHandler &msglogerr) : 
    server{NULL}, msglogerr(msglogerr) {}

NetMain::~NetMain() {}

NetMode NetMain::getNetType() {
    return NetMain::NetType;
}

void NetMain::setNetType(NetMode NetType) {
    NetMain::NetType = NetType;
}

}