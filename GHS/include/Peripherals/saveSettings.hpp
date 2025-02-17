#ifndef SAVESETTINGS_HPP
#define SAVESETTINGS_HPP

#include "NVS2/NVS.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Soil.hpp"
#include "Config/config.hpp"

namespace NVS {

#define NVS_NAMESPACE_SAVE "settings" // 14 char max

// NVS keys (14 char max)
#define TEMP_KEY "tempSave"
#define HUM_KEY "humSave"
#define SOIL_KEY "soilSave"

struct configSaveReTimer { // Relay Timer configurations.
    bool enabled; // AKA is ready.
    uint32_t onTime; // When relay is set to turn on.
    uint32_t offTime; // When relay is set to turn off.
}; // x4

struct configSaveReAltTH { // Relay & Alt config Temp Hum.
    uint8_t relayNum; // Relay number attached.
    uint8_t relayCond; // Relay condition.
    int relayTripVal; // Relay trip value.
    uint8_t altCond; // Alert condition.
    int altTripVal; // Alert Trip value.
}; // x2

struct configSaveAltSoil { // Alert config soil sensors.
    uint8_t cond; // Alert condition.
    int tripVal; // Alert trip value.
}; // x4

struct configSaveReLight { // Relay config photoresistor.
    uint8_t relayNum; // Relay number attached
    uint8_t relayCond; // Relay condition.
    uint16_t relayTripVal; // Relay trip value.
    uint16_t darkVal; // Dark value for light duration computation.
}; // x1

struct configSaveMaster {
    configSaveReTimer relays[TOTAL_RELAYS]; 
    configSaveReAltTH temp, hum;
    configSaveAltSoil soil[SOIL_SENSORS];
    configSaveReLight light;
};

class settingSaver { // Singleton
    private:
    static Threads::Mutex mtx;
    NVSctrl nvs;
    configSaveMaster master;
    settingSaver(); 
    settingSaver(const settingSaver&) = delete; // prevent copying
    settingSaver &operator=(const settingSaver&) = delete; // prevent assignment
    void checkTH();
    void checkRelayTimers();
    void checkSoil();
    void checkLight();
    template <typename T>
    bool compare(T& curVal, const T& newVal) {
        if (curVal != newVal) {
            curVal = newVal;
            return false; // Indicates a rewrite must occur
        }
        return true; // Values are equal
    }

    public:
    static settingSaver* get();
    void save();
    void load();
};



}

#endif // SAVESETTINGS_HPP