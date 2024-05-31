// #include "Network.h"
// #include <ArduinoJson.h>
// #include <ESP8266mDNS.h>

// Net::Net(
//     const char* AP_SSID, 
//     const char* AP_Pass, 
//     IPAddress local_IP,
//     IPAddress gateway,
//     IPAddress subnet,
//     uint16_t port) : 

//     AP_SSID(AP_SSID),
//     AP_Pass(AP_Pass),
//     local_IP(local_IP),
//     gateway(gateway),
//     subnet(subnet),
//     server(BearSSL::ESP8266WebServerSecure(port)),
//     port(port),
//     prevServerType(NO_WIFI),
//     MDNSrunning(false),
//     isServerRunning(false)
//     {
//         // Set memory to all null for char arrays
//         memset(this->ST_SSID, 0, sizeof(this->ST_SSID));
//         memset(this->ST_PASS, 0, sizeof(this->ST_PASS));
//         memset(this->phoneNum, 0, sizeof(this->phoneNum));
//         memset(this->error, 0, sizeof(this->error));

//         // RESTRUCTURE THE NETWORK TO SMALLER CPP FILES
//         // FIGURE ALL OF THIS OUT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//         Serial.println("SETTING UP NETWORK");
//         // Start the file system
//         if(!LittleFS.begin()) {
//             strcpy(this->error, "failed LittleFS mount");
//             Serial.println("LITTLE FS ERROR"); return;
//             // OLED.displayError(this->error);
//         } 

//         // Load the file certificate
//         File cert = LittleFS.open("/certificate.der", "r");
//         if(!cert) {
//             strcpy(this->error, "Failed to open security cert");
//             Serial.println("cert ERROR"); return;
//             // OLED.displayError(this->error); return;
//         }

//         size_t certSize = cert.size();
//         uint8_t* certBuffer = new uint8_t[certSize];
//         if (cert.read(certBuffer, certSize) == certSize) {
//             this->serverCert.append(certBuffer, certSize);
//         }
//         delete[] certBuffer; cert.close();

//         // Load the private key
//         File privateKey = LittleFS.open("/private_key.der", "r");
//         if (!privateKey) {
//             strcpy(this->error, "Failed to open private key");
//             Serial.println("KEy ERROR"); return;
//             // OLED.displayError(this->error); return;
//         }

//         size_t keySize = privateKey.size();
//         uint8_t* keyBuffer = new uint8_t[keySize];
//         if (privateKey.read(keyBuffer, keySize) == keySize) {
//             this->serverPrivateKey.parse(keyBuffer, keySize);
//         }
//         delete[] keyBuffer; privateKey.close();

//         this->server.getServer().setRSACert(&this->serverCert, &this->serverPrivateKey);
//     } 

// // WIRELESS ACCESS POINT EXCLUSIVE. This will host the same exact page as the station
// // which is the main control for the device. It will just do so with the controlling
// // device connects to the device via wifi. Will not have any connection internet.

// bool Net::WAP(IDisplay &OLED) { 
//     // check if already in AP mode, setup if not
//     Serial.println("ON WAP MODE");
//     if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_ONLY) {
//         prevServerType = WAP_ONLY;
//         WiFi.mode(WIFI_AP);
//         WiFi.softAPConfig(local_IP, gateway, subnet);
//         WiFi.softAP(AP_SSID, AP_Pass);
//         if (!this->isServerRunning) startServer(OLED);
//         if (MDNSrunning) {
//             MDNS.end(); MDNSrunning = false;
//         }
//         return false; // connecting
//     } else {
//         return true; // running
//     }
// }

// // WIRELESS ACCESS POINT TO SETUP LAN CONNECTION. This is designed to broadcast via
// // wifi, the setup page where the user can enter their SSID, password, and phone 
// // for a twilio account altert systems. Once this is set up, the client can go 
// // to station mode.

// bool Net::WAPSetup(IDisplay &OLED) {
//     // check if already in AP mode, setup if not
//     if (WiFi.getMode() != WIFI_AP || prevServerType != WAP_SETUP) {
//         prevServerType = WAP_SETUP;
//         WiFi.mode(WIFI_AP);
//         WiFi.softAPConfig(local_IP, gateway, subnet);
//         WiFi.softAP(AP_SSID, AP_Pass);
//         if (!this->isServerRunning) startServer(OLED);
//         if (MDNSrunning) {
//             MDNS.end(); MDNSrunning = false;
//         }
//         return false; // connecting
//     } else {
//         return true; // running
//     }
// }

// // STATION IS THE IDEAL MODE. This is deisgned to allow the user to view the controller
// // webpage from any device connected to the LAN. This will also have access to the internet
// // for SMS updates and alerts, as well as to check for upgraded firmware. 

// uint8_t Net::STA(IDisplay &OLED) {
    
//     if (WiFi.getMode() != WIFI_STA || prevServerType != STA_ONLY) {
//         // resetServer();
//         WiFi.mode(WIFI_STA);
//         this->prevServerType = STA_ONLY;

//         // Checks if this has already been created by the WAP setup page. This will
//         // happen upon attempting a write to the EEPROM of the SSID, PASS, and phone.
//         // This serves as a catch for an EEPROM failure. This allows the user to 
//         // sign in at the WAP setup page, which stores the variables, and then will
//         // connect. The EEPROM serves to provide this data upon startup and is the 
//         // primary. A reminder though, without EEPROM, the LAN login creds are volatile.

//         if (this->ST_SSID[0] == '\0' || this->ST_PASS[0] == '\0') {
//             STAsettings readEEPROM; // reads eeprom to get stored vals up creation
//             readEEPROM.eepromRead(0); // reads epprom data and sets class variables

//             // The readEEPROM returns char pointers, which will go out of scope and 
//             // cause issues after initialization in this function. To prevent that and 
//             // store the class variables, we use these functions, and then have no 
//             // follow on issues. These need to be mutable.
//             strncpy(this->ST_SSID, readEEPROM.getSSID(), sizeof(this->ST_SSID) - 1);
//             this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0'; // Ensure null terminator exists

//             strncpy(this->ST_PASS, readEEPROM.getPASS(), sizeof(this->ST_PASS) - 1);
//             this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';

//             strncpy(this->phoneNum, readEEPROM.getPhone(), sizeof(this->phoneNum) - 1);
//             this->phoneNum[sizeof(this->phoneNum) - 1] = '\0';
//         } 
        
//         WiFi.begin(this->ST_SSID, this->ST_PASS);
//         if (!this->isServerRunning) startServer(OLED);

//         if (WiFi.status() == WL_CONNECTED) {
//             if (!MDNSrunning) {
//                 MDNS.begin("esp8266"); MDNSrunning = true;
//             }
//             return WIFI_RUNNING;
//         } else {
//             return WIFI_STARTING;
//         }
//     } else {
//         if (WiFi.status() == WL_CONNECTED) {
//             if (!MDNSrunning) {
//                 MDNS.begin("esp8266"); MDNSrunning = true;
//             }
//             return WIFI_RUNNING;
//         } else {
//             return WIFI_STARTING;
//         }
//     }
// }

// // This has to have all of the servers in here. The intention is to 
// // setup all routes at once, to avoid redundancy, as well as begin the
// // server to handle the client.

// void Net::startServer(IDisplay &OLED) {
//     auto handleIndex = [this]() {
//         if (this->prevServerType == WAP_ONLY) {
//             server.send(200, "text/html", "THIS IS WAP ONLY");
//         } else if (this->prevServerType == WAP_SETUP) {
//             server.send(200, "text/html", WAPsetup);
//         } else if (this->prevServerType == STA_ONLY) {
//             server.send(200, "text/html", "THIS IS STATION ONLY");
//         }
//     };

//     // ADD ENCRYPTION AS WELL AS HTTPS FOR WAPSETUP SERVER
//     auto handleWAPsubmit = [this, &OLED]() {
//         if (this->prevServerType == WAP_SETUP) {
//             if (server.hasArg("plain")) {
//                 String jsonData; jsonData.reserve(256);
//                 String ssid; ssid.reserve(32);
//                 String pass; pass.reserve(64);
//                 String phone; phone.reserve(15);
//                 String response; response.reserve(64);
                
//                 jsonData = server.arg("plain");
//                 DynamicJsonDocument jsonDoc(256);

//                 if (deserializeJson(jsonDoc, jsonData)) { // shows error if true
//                     strcpy(this->error, "JSON deserialization ERR");
//                     OLED.displayError(this->error); return; // prevents further issues
//                 };

//                 ssid = jsonDoc["ssid"].as<String>();
//                 pass = jsonDoc["pass"].as<String>();
//                 phone = jsonDoc["phone"].as<String>();

//                 STAsettings writeToEEPROM;
//                 DynamicJsonDocument res(64); // sends back to client

//                 // This is done during the initial save of the variables to EEPROM. This
//                 // exists to serve as a backup in the event EEPROM fails. If it does fail
//                 // The response to the client will be TScode:1, which means EEPROM fail. 
//                 // The EEPROM will fail when the read back != the input, but it will still 
//                 // allow the user to connect to the LAN. However, this is volatile memory 
//                 // and will need to be done upon every powerdown, but its doable while waiting
//                 // for a replacement. 
//                 strncpy(this->ST_SSID, ssid.c_str(), sizeof(this->ST_SSID) - 1);
//                 this->ST_SSID[sizeof(this->ST_SSID) - 1] = '\0';

//                 strncpy(this->ST_PASS, pass.c_str(), sizeof(this->ST_PASS) - 1);
//                 this->ST_PASS[sizeof(this->ST_PASS) - 1] = '\0';

//                 strncpy(this->phoneNum, phone.c_str(), sizeof(this->phoneNum) - 1);
//                 this->phoneNum[sizeof(this->phoneNum) - 1] = '\0';

//                 if(writeToEEPROM.eepromWrite(ssid, pass, phone)) {
//                     res["status"] = "Received, swap to station mode to check connection";
//                 } else {
//                     res["status"] = "Error storing data, swap to station (TScode:1)";
//                 }

//                 serializeJson(res, response);
//                 server.send(200, "application/json", response);
//             } else {
//                 server.send(400, "text/plain", "Bad Request. No JSON payload!");
//             }
//         } else {
//             server.send(404, "text/html", "UNAUTHORIZED FROM SERVER");
//         }
//     };

//     auto handleNotFound = [this]() {
//         server.send(404, "text/html", "Page Not Found");
//     };

//     server.on("/", handleIndex);
//     server.on("/WAPsubmit", handleWAPsubmit);
//     server.onNotFound(handleNotFound);
//     server.begin(); 
//     this->isServerRunning = true;

// }

// STAdetails Net::getSTADetails() {
//     STAdetails details;
//     strcpy(details.SSID, this->ST_SSID);
//     IPAddress ip = WiFi.localIP();
//     sprintf(details.IPADDR, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
//     sprintf(details.signalStrength, "%d dBm", WiFi.RSSI());
//     return details;
// }

// void Net::handleServer() {
//     if (this->isServerRunning) { // begins after server.begin()
//         this->server.handleClient();
//     }
// }

// uint8_t netSwitch() {
//     // 0 = WAP exclusive, 1 = WAP Program, 2 = Station
//     uint8_t WAP = digitalRead(WAPswitch);
//     uint8_t STA = digitalRead(STAswitch);    

//     if (!WAP && STA) { // WAP Exclusive
//         return WAP_ONLY;
//     } else if (WAP && STA) { // Middle position, WAP Program mode for STA
//         return WAP_SETUP;
//     } else if (!STA && WAP) { // Station mode reading from EEPROM
//         return STA_ONLY;
//     } else {
//         return NO_WIFI; // standard error throughout this program
//     }
// }

