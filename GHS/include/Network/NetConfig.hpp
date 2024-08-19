#ifndef NETCONFIG_HPP
#define NETCONFIG_HPP

namespace Comms {

enum class NetMode {
    WAP, WAP_SETUP, STA, NONE, WAP_RECON
};

enum class Constat {
    CON, DISCON
};

enum class IDXSIZE {
    SSID = 32,
    PASS = 64,
    PHONE = 15,
    NETCREDKEYQTY = 4,
    IPADDR = 16,
    MDNS = 11
};

enum class KI {ssid, pass, phone, WAPpass}; // Key Index
extern const char* netKeys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)];

}

#endif // NETCONFIG_HPP