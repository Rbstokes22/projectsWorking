#include "Peripherals.h"

namespace Devices {

TempHum::TempHum(uint8_t pin, uint8_t type) : 
    dht{pin, type}
{
    this->dht.begin();
}

float TempHum::getTemp() {
    return this->dht.readTemperature();
}

float TempHum::getHum() {
    return this->dht.readHumidity();
}

Light::Light(uint8_t photoResistorPin) : 

currentLight{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
lightAccumulation{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
photoResistorPin{photoResistorPin}{}

void Light::begin() {
    this->as7341.begin();
}

void Light::readAndSet() {
    this->as7341.readAllChannels();
    this->currentLight.violet = 
        this->as7341.getChannel(AS7341_CHANNEL_415nm_F1);
    this->currentLight.indigo = 
        this->as7341.getChannel(AS7341_CHANNEL_445nm_F2);
    this->currentLight.blue = 
        this->as7341.getChannel(AS7341_CHANNEL_480nm_F3);
    this->currentLight.cyan = 
        this->as7341.getChannel(AS7341_CHANNEL_515nm_F4);
    this->currentLight.green = 
        this->as7341.getChannel(AS7341_CHANNEL_555nm_F5);
    this->currentLight.yellow = 
        this->as7341.getChannel(AS7341_CHANNEL_590nm_F6);
    this->currentLight.orange = 
        this->as7341.getChannel(AS7341_CHANNEL_630nm_F7);
    this->currentLight.red = 
        this->as7341.getChannel(AS7341_CHANNEL_680nm_F8);
    this->currentLight.nir = 
        this->as7341.getChannel(AS7341_CHANNEL_NIR);
    this->currentLight.clear = 
        this->as7341.getChannel(AS7341_CHANNEL_CLEAR);
    this->currentLight.flickerHz = 
        this->as7341.detectFlickerHz();
}

uint64_t Light::getColor(const char* color) {
    if (strcmp(color, "violet") == 0) return this->currentLight.violet;
    if (strcmp(color, "indigo") == 0) return this->currentLight.indigo;
    if (strcmp(color, "blue") == 0) return this->currentLight.blue;
    if (strcmp(color, "cyan") == 0) return this->currentLight.cyan;
    if (strcmp(color, "green") == 0) return this->currentLight.green;
    if (strcmp(color, "yellow") == 0) return this->currentLight.yellow;
    if (strcmp(color, "orange") == 0) return this->currentLight.orange;
    if (strcmp(color, "red") == 0) return this->currentLight.red;
    if (strcmp(color, "nir") == 0) return this->currentLight.nir;
    if (strcmp(color, "clear") == 0) return this->currentLight.clear;
    return 65536; // shows that the color passed was not in the scope of this function
}

uint64_t Light::getFlicker() {
    return this->currentLight.flickerHz;
}

void Light::getAccumulation() {

}

void Light::clearAccumulation() {
    lightAccumulation = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
}

uint16_t Light::getLightIntensity() {
    return analogRead(this->photoResistorPin);
}

SensorObjects::SensorObjects(
    Devices::TempHum &tempHum,
    Devices::Light &light,
    Clock::Timer &checkSensors) : 

    tempHum{tempHum}, light{light}, checkSensors{checkSensors}{}

}

void handleSensors(
    Devices::TempHum &tempHum, Devices::Light &light
    ) { 
    Serial.println("--------------------------");
    light.readAndSet();
    Serial.print("Temp: "); Serial.println(tempHum.getTemp());
    Serial.print("Hum: "); Serial.println(tempHum.getHum());
    Serial.print("Red: "); Serial.println(light.getColor("red"));
    // light.readAndSet();
    // Serial.print("red counts: "); Serial.println(light.getColor("red"));
    // Serial.print("nir counts: "); Serial.println(light.getColor("nir"));
    // Serial.print("Temp: "); Serial.println(tempHum.getTemp());
    // Serial.print("Hum: "); Serial.println(tempHum.getHum());




    // as7341.readAllChannels();
    // Serial.println("================================");
    // Serial.print("Photo: "); Serial.println(analogRead(Photoresistor_PIN));
    // Serial.print("TEMP: "); Serial.println(dht.readTemperature());
    // Serial.print("HUM: "); Serial.println(dht.readHumidity());
    // Serial.print("Soil1: "); Serial.println((analogRead(Soil_1_PIN)));


    // Serial.print("F1: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_415nm_F1));
    // Serial.print("F2: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_445nm_F2));
    // Serial.print("F3: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_480nm_F3));
    // Serial.print("F4: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_515nm_F4));
    // Serial.print("F5: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_555nm_F5));
    // Serial.print("F6: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_590nm_F6));
    // Serial.print("F7: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_630nm_F7));
    // Serial.print("F8: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_680nm_F8));
    // Serial.print("Clear: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_CLEAR));
    // Serial.print("NIR: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_NIR));

}