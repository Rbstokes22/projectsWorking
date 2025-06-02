#ifndef SAVESETTINGS_HPP
#define SAVESETTINGS_HPP

#include "NVS2/NVS.hpp"
#include "Threads/Mutex.hpp"
#include "Peripherals/Soil.hpp"
#include "Config/config.hpp"
#include "Peripherals/Relay.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Peripherals/Alert.hpp"
#include "Peripherals/Light.hpp"

// NVS carriers are in the master struct. These carriers are responsible to
// communicate with the NVS, so current data is copied to these structs, and
// these structs are written to NVS. When loading data, data is read from NVS
// and populated into these structs, and then the values are copied over to 
// the current settings. NOTE: When saving data, if the current values equal
// the struct values, the NVS will not be written. This prevents attempts to 
// re-write the same values to NVS and acts as a first-layer gate. Each sensor
// has its own key with its own values.

namespace NVS {

#define NVS_NAMESPACE_SAVE "settings" // 14 char max
#define SAVESETTING_ERR_METHOD Messaging::Method::SRL_LOG

// NVS keys (12 char max) used to save each peripheral setting.
#define TEMP_KEY "tempSave"
#define HUM_KEY "humSave"
#define SOIL1_KEY "soil1Save" // Names can either start from 0 or 1.
#define SOIL2_KEY "soil2Save"
#define SOIL3_KEY "soil3Save"
#define SOIL4_KEY "soil4Save"
#define LIGHT_KEY "lightSave"
#define RELAY1_KEY "relay1Save"
#define RELAY2_KEY "relay2Save"
#define RELAY3_KEY "relay3Save"
#define RELAY4_KEY "relay4Save"
#define SETTINGS_TAG "(SETTINGS)"

#define NVS_SETTING_DELAY 20 // millis to delay between NVS read/write calls.

struct configSaveReTimer { // Relay Timer configurations.
    uint32_t onTime; // When relay is set to turn on.
    uint32_t offTime; // When relay is set to turn off.
    uint8_t days; // Days when the relay will be used.
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
    AS7341_DRVR::AGAIN AGAIN; // AGAIN values for the spec sensor.
    uint8_t ATIME; // ATIME values for the spec sensor.
    uint16_t ASTEP; // ASTEP values for the spec sensor.
}; // x1

// Composite class consisting of structs for individual device.
struct configSaveMaster {
    configSaveReTimer relays[TOTAL_RELAYS]; 
    configSaveReAltTH temp, hum;
    configSaveAltSoil soil[SOIL_SENSORS];
    configSaveReLight light;
};

class settingSaver { // Singleton
    private:
    static Threads::Mutex mtx;
    const char* relayKeys[TOTAL_RELAYS]; // All relay NVS keys.
    const char* soilKeys[SOIL_SENSORS]; // All soil sensor NVS keys.
    const char* tag; // Used in logging.
    char log[LOG_MAX_ENTRY]; // Used to log/print entries.
    nvs_ret_t err; // Error of nvs read/write returns.
    NVSctrl nvs; // NVS controller
    configSaveMaster master; // Master with all sub-structs
    size_t expected, total; // Set in each save method.
    Peripheral::Relay* relays; // Pointer to all relays.
    settingSaver(); 
    settingSaver(const settingSaver&) = delete; // prevent copying
    settingSaver &operator=(const settingSaver&) = delete; // prevent assignment
    bool saveTH();
    bool saveRelayTimers();
    bool saveSoil();
    bool saveLight();
    bool loadTH();
    bool loadRelayTimers();
    bool loadSoil();
    bool loadLight();
    void sendErr(const char* msg, Messaging::Levels level = 
        Messaging::Levels::ERROR);

    // Compare function to test current values against new values. If !=,
    // copies to new value to the current value and returns false indicating
    // a rewrite is necessary.
    template <typename T>
    bool compare(T& curVal, const T& newVal) {
    
        if (curVal != newVal) {
            curVal = newVal; // Copy new value to running value if !=.
            return false; // Signals a rewrite must occur
        }

        return true; // Values are equal
    }

    public:
    static settingSaver* get();
    bool save();
    bool load();
    void initRelays(Peripheral::Relay* relayArray);
};

}

#endif // SAVESETTINGS_HPP