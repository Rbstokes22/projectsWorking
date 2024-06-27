#ifndef NETCONFIG_H
#define NETCONFIG_H

namespace Comms {

enum class IDXSIZE {
SSID = 32,
IPADDR = 16,
SIGSTRENGTH = 16,
PASS = 64, 
PHONE = 15,
NETCREDKEYQTY = 4, // uses for ssid, pass, phone, and WAPpass
KEYQTYSTA = 3, // uses only ssid, pass, and phone
};

enum class KI {ssid, pass, phone, WAPpass};

extern const char* keys[static_cast<int>(IDXSIZE::NETCREDKEYQTY)];

}


#endif // NETCONFIG_H