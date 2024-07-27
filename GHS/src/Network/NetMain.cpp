#include "Network/NetMain.hpp"

namespace Comms {

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

void NetMain::sendErr(const char* msg) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR, msg, 
        Messaging::Method::OLED, 
        Messaging::Method::SRL);
}

}