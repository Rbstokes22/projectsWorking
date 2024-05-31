#include "Network.h"
#include <FS.h>
#include <LittleFS.h>
#include <memory>


void Net::loadCertificates() {
    
    Serial.println("SETTING UP NETWORK");
    // Start the file system
    if(!LittleFS.begin()) {
        strcpy(this->error, "failed LittleFS mount");
        Serial.println("LITTLE FS ERROR"); return;
        // OLED.displayError(this->error);
    } 

    // Load the file certificate
    File cert = LittleFS.open("/certificate.der", "r");
    if(!cert) {
        strcpy(this->error, "Failed to open security cert");
        Serial.println("cert ERROR"); return;
        // OLED.displayError(this->error); return;
    }

    size_t certSize = cert.size();
    std::unique_ptr<uint8_t[]> certBuffer(new uint8_t[certSize]);
    if (cert.read(certBuffer.get(), certSize) == certSize) {
        memcpy(this->serverCert, certBuffer.get(), certSize);
        this->serverCert = certBuffer.get();
    }
    cert.close();

    // Load the private key
    File privateKey = LittleFS.open("/private_key.der", "r");
    if (!privateKey) {
        strcpy(this->error, "Failed to open private key");
        Serial.println("Key ERROR"); return;
        // OLED.displayError(this->error); return;
    }

    size_t keySize = privateKey.size();
    std::unique_ptr<uint8_t[]> keyBuffer(new uint8_t[keySize]);
    if (privateKey.read(keyBuffer.get(), keySize) == keySize) {
        this->serverPrivateKey.parse(keyBuffer.get(), keySize);
    }
    privateKey.close();

    // this->server.getServer().setRSACert(&this->serverCert, &this->serverPrivateKey);
}