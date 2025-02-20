#ifndef SAVESETTINGS_HPP
#define SAVESETTINGS_HPP

#include "NVS2/NVS.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Soil.hpp"
#include "Config/config.hpp"
#include "Peripherals/Relay.hpp"

namespace NVS {

#define NVS_NAMESPACE_SAVE "settings" // 14 char max

// NVS keys (14 char max)
#define TEMP_KEY "tempSave"
#define HUM_KEY "humSave"
#define SOIL1_KEY "soil1Save"
#define SOIL2_KEY "soil2Save"
#define SOIL3_KEY "soil3Save"
#define SOIL4_KEY "soil4Save"
#define LIGHT_KEY "lightSave"
#define RELAY1_KEY "relay1Save"
#define RELAY2_KEY "relay2Save"
#define RELAY3_KEY "relay3Save"
#define RELAY4_KEY "relay4Save"

struct configSaveReTimer { // Relay Timer configurations.
    uint32_t onTime; // When relay is set to turn on.
    uint32_t offTime; // When relay is set to turn off.
}; // x4

struct configSaveReAltTH { // Relay & Alt config Temp Hum.
    uint8_t relayNum; // Relay number attached.
    Peripheral::RECOND relayCond; // Relay condition.
    int relayTripVal; // Relay trip value.
    Peripheral::ALTCOND altCond; // Alert condition.
    int altTripVal; // Alert Trip value.
}; // x2

struct configSaveAltSoil { // Alert config soil sensors.
    Peripheral::ALTCOND cond; // Alert condition.
    int tripVal; // Alert trip value.
}; // x4

struct configSaveReLight { // Relay config photoresistor.
    uint8_t relayNum; // Relay number attached
    Peripheral::RECOND relayCond; // Relay condition.
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
    size_t expected, total;
    Peripheral::Relay* relays;
    settingSaver(); 
    settingSaver(const settingSaver&) = delete; // prevent copying
    settingSaver &operator=(const settingSaver&) = delete; // prevent assignment
    void saveTH();
    void saveRelayTimers();
    void saveSoil();
    void saveLight();
    void loadTH();
    void loadRelayTimers();
    void loadSoil();
    void loadLight();
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
    void initRelays(Peripheral::Relay* relayArray);
};

}

#endif // SAVESETTINGS_HPP