#ifndef NETCONFIG_HPP
#define NETCONFIG_HPP

namespace Comms {

enum class NetMode {
    WAP, WAP_SETUP, STA, NONE
};

enum class Constat {
    CON, DISCON
};

enum class IDXSIZE {
    SSID = 32,
    PASS = 64,
    PHONE = 15,
    NETCREDKEYQTY = 4
};

enum class KI {ssid, pass, phone, APpass}; // Key Index
extern const char* keys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)];

}

#endif // NETCONFIG_HPP