#ifndef NETWORK_H
#define NETWORK_H

#include <ESP8266WebServerSecure.h>
#include "IDisplay.h"

// For secure webserver you can use openssl to make .der files which is a binary
// that the esp accepts. Create and store these in a data directory in the 
// platformio file directory. Configure the platformio.ini file to use the 
// appropriate file system wither SPIFFS or LittleFS.

// Switches
#define WAPswitch D6 // Wireless Access Point
#define STAswitch D5 // Station

// Keywords
#define WAP_ONLY 0
#define WAP_SETUP 1
#define STA_ONLY 2
#define NO_WIFI 255
#define WIFI_STARTING 4
#define WIFI_RUNNING 5

struct STAdetails {
    char SSID[32];
    char IPADDR[16];
    char signalStrength[16];
};

class Net {
    private:
    const char* AP_SSID;
    const char* AP_Pass;
    char ST_SSID[32];
    char ST_PASS[64];
    char phoneNum[15];
    IPAddress local_IP;
    IPAddress gateway;
    IPAddress subnet;
    BearSSL::ESP8266WebServerSecure server;
    BearSSL::X509List serverCert;
    BearSSL::PrivateKey serverPrivateKey;
    uint16_t port;
    uint8_t prevServerType;
    bool MDNSrunning;
    char error[40];
    bool isServerRunning;

    public:
    Net(
        const char* AP_SSID, 
        const char* AP_Pass,
        IPAddress local_IP,
        IPAddress gateway,
        IPAddress subnet,
        uint16_t port
        );

    void loadCertificates();
    bool WAP(IDisplay &OLED);
    bool WAPSetup(IDisplay &OLED); // Setup LAN setting from WAP
    uint8_t STA(IDisplay &OLED);
    void startServer(IDisplay &OLED);
    STAdetails getSTADetails(); // returns the struct STAdetails
    void handleServer();
    
};

uint8_t netSwitch(); // Checks the 3-way switch positon for the correct mode

const char WAPsetup[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WAP Programming</title>
    <style>
        h1 {
            text-align: center;
            font-weight: 900;
        }

        .p1 {
            font-size: 1.5em;
            font-weight: 700;
        }

        .p2 {
            background-color: yellow;
            padding:5px;
        }
    </style>
</head>
<body>
    <h1>GreenHouse LAN setup</h1><br>
    <p class="p1">Required:</p>
    <input id="ssid" placeholder="Network Name">
    <input id="pass" placeholder="Password">
    <br><br>
    <p class="p1">Optional: (10 digit phone)</p>
    <input id="phone" value="0123456789"><br>
    <p class=p2>
        Note* Based on subscription you will be able to receive daily reports,
        receive critical alerts, and make changes to your system while away via SMS. 
        This is a paid service available on the website.
    </p>
    <button onclick=submit()>Submit</button><br><br>
    <p class="p1">Status:</p>
    <p id="status"></p>

    <script>
        // Allows the form to meet requirements before submission.
        let validate = () => {
            let count = 0;
            const ssid = document.getElementById("ssid");
            const pass = document.getElementById("pass");
            const phone = document.getElementById("phone");
            const status = document.getElementById("status");

            if (!ssid.value) {
                ssid.style.border = "2px solid red";
            } else {
                ssid.style.border = "1px solid black";
                count++;
            }

            if (!pass.value) {
                pass.style.border = "2px solid red";
            } else {
                pass.style.border = "1px solid black";
                count++;
            }

            // Leaves only digits
            phoneNum = phone.value.replace(/\D/g, '').toString();

            if(phoneNum[0] === '1') {
                phoneNum = phoneNum.substring(1);
;            }

            if(phoneNum.length != 10) {
                console.log(phoneNum.length);
                phone.style.border = "2px solid red";
            } else {
                phone.style.border = "1px solid black";
                count++;
            }

            if (count === 3) { 
                return true;
            } else {
                return false;
            }
        }
        
        let submit = () => {
            const ssid = document.getElementById("ssid");
            const pass = document.getElementById("pass");
            const phone = document.getElementById("phone");
            const status = document.getElementById("status");

            if (validate()) {
                fetch("http://192.168.1.1/WAPsubmit", {
                method: "POST",
                headers: {"Content-Type" : "application/json"},
                body: JSON.stringify({
                    ssid:ssid.value,
                    pass:pass.value,
                    phone:phone.value
                })})
                .then(response => response.json())
                .then(response => {
                    status.innerText = response.status;
                    ssid.value = "";
                    pass.value = "";
                    phone.value = "0123456789";
                })
                .catch(err => {console.log(err.message)});
            } else {
                status.innerHTML = 
                `<b>ERRORS IN RED:</b><br>
                Network/SSID & pass required<br>
                Phone must be 10 digits`;
            }
        }
    </script>
</body>
</html>
)rawliteral";

#endif // NETWORK_H