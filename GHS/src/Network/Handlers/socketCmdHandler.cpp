#include "Network/Handlers/socketHandler.hpp"
#include "esp_http_server.h"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Config/config.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Soil.hpp"
#include "Common/Timing.hpp"

namespace Comms {

void SOCKHAND::compileData(cmdData &data, char* buffer, size_t size) {
    int written{0};
    const char* reply = "{\"status\":%d,\"msg\":\"%s\",\"supp\":%d}";
    
    // All commands work from here.
    switch(data.cmd) {

        case CMDS::GET_ALL: {
        int soilReadings[SOIL_SENSORS] = {0, 0, 0, 0};

        Clock::DateTime* dtg = Clock::DateTime::get();
        Clock::TIME* time = dtg->getTime();
        Peripheral::TempHum* th = Peripheral::TempHum::get();
        Peripheral::Timer* re1Timer = SOCKHAND::Relays[0].getTimer();
        Peripheral::Timer* re2Timer = SOCKHAND::Relays[1].getTimer();
        Peripheral::Timer* re3Timer = SOCKHAND::Relays[2].getTimer();
        Peripheral::Timer* re4Timer = SOCKHAND::Relays[3].getTimer();
        Peripheral::Soil* soil = Peripheral::Soil::get();
        soil->getAll(soilReadings, sizeof(int) * SOIL_SENSORS);
        
        written = snprintf(buffer, size,  
        "{\"firmv\":\"%s\",\"id\":\"%s\",\"sysTime\":%zu,\"hhmmss\":\"%d:%d:%d\","
        "\"timeCalib\":%d,"
        "\"re1\":%d,\"re1TimerEn\":%d,\"re1TimerOn\":%zu,\"re1TimerOff\":%zu,"
        "\"re2\":%d,\"re2TimerEn\":%d,\"re2TimerOn\":%zu,\"re2TimerOff\":%zu,"
        "\"re3\":%d,\"re3TimerEn\":%d,\"re3TimerOn\":%zu,\"re3TimerOff\":%zu,"
        "\"re4\":%d,\"re4TimerEn\":%d,\"re4TimerOn\":%zu,\"re4TimerOff\":%zu,"
        "\"temp\":%.2f,\"tempRelay\":%d,\"tempCond\":%u,\"tempRelayVal\":%d,"
        "\"tempAlertVal\":%d,\"tempAlertEn\":%d,"
        "\"hum\":%.2f,\"humRelay\":%d,\"humCond\":%u,\"humRelayVal\":%d,"
        "\"humAlertVal\":%d,\"humAlertEn\":%d,\"SHTUp\":%d,"
        "\"soil1\":%d,\"soil1Cond\":%u,\"soil1AlertVal\":%d,\"soil1AlertEn\":%d,"
        "\"soil2\":%d,\"soil2Cond\":%u,\"soil2AlertVal\":%d,\"soil2AlertEn\":%d,"
        "\"soil3\":%d,\"soil3Cond\":%u,\"soil3AlertVal\":%d,\"soil3AlertEn\":%d,"
        "\"soil4\":%d,\"soil4Cond\":%u,\"soil4AlertVal\":%d,\"soil4AlertEn\":%d,"
        "\"soil1Up\":%d,\"soil2Up\":%d,\"soil3Up\":%d,\"soil4Up\":%d}",
        FIRMWARE_VERSION, 
        data.idNum,
        static_cast<size_t>(time->raw),
        time->hour, time->minute, time->second,
        dtg->isCalibrated(),
        static_cast<uint8_t>(SOCKHAND::Relays[0].getState()),
        re1Timer->isReady, (size_t)re1Timer->onTime, (size_t)re1Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[1].getState()),
        re2Timer->isReady, (size_t)re2Timer->onTime, (size_t)re2Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[2].getState()),
        re3Timer->isReady, (size_t)re3Timer->onTime, (size_t)re3Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[3].getState()),
        re4Timer->isReady, (size_t)re4Timer->onTime, (size_t)re4Timer->offTime,
        th->getTemp(),
        th->getTempConf()->relayNum,
        static_cast<uint8_t>(th->getTempConf()->condition),
        th->getTempConf()->tripValRelay,
        th->getTempConf()->tripValAlert,
        th->getTempConf()->alertsEn,
        th->getHum(),
        th->getHumConf()->relayNum,
        static_cast<uint8_t>(th->getHumConf()->condition),
        th->getHumConf()->tripValRelay,
        th->getHumConf()->tripValAlert,
        th->getHumConf()->alertsEn,
        th->getStatus().display,
        soilReadings[0], static_cast<uint8_t>(soil->getConfig(0)->condition),
        soil->getConfig(0)->tripValAlert, soil->getConfig(0)->alertsEn,
        soilReadings[1], static_cast<uint8_t>(soil->getConfig(1)->condition),
        soil->getConfig(1)->tripValAlert, soil->getConfig(1)->alertsEn,
        soilReadings[2], static_cast<uint8_t>(soil->getConfig(2)->condition),
        soil->getConfig(2)->tripValAlert, soil->getConfig(2)->alertsEn,
        soilReadings[3], static_cast<uint8_t>(soil->getConfig(3)->condition),
        soil->getConfig(3)->tripValAlert, soil->getConfig(3)->alertsEn,
        soil->getStatus(0)->display, soil->getStatus(1)->display,
        soil->getStatus(2)->display, soil->getStatus(3)->display
        );
        }
        break;

        case CMDS::CALIBRATE_TIME: 
        if (!SOCKHAND::inRange(0, 86399, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, 
            "Time out of range", data.suppData);
        } else {
            Clock::DateTime::get()->calibrate(data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Time calibrated to", data.suppData);
        }
        
        break;

        case CMDS::RELAY_1: // inRange not required here due to else block.
        static uint8_t IDR1 = SOCKHAND::Relays[0].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[0].off(IDR1);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[0].on(IDR1);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[0].forceOff();
        } else {
            SOCKHAND::Relays[0].removeForce();
        }

        written = snprintf(buffer, size, reply, 1, "Re1 set", data.suppData);

        break;

        case CMDS::RELAY_2: // inRange not required here due to else block.
        static uint8_t IDR2 = SOCKHAND::Relays[1].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[1].off(IDR2);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[1].on(IDR2);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[1].forceOff();
        } else {
            SOCKHAND::Relays[1].removeForce();
        }

        written = snprintf(buffer, size, reply, 1, "Re2 set", data.suppData);

        break;

        case CMDS::RELAY_3: // inRange not required here due to else block.
        static uint8_t IDR3 = SOCKHAND::Relays[2].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[2].off(IDR3);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[2].on(IDR3);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[2].forceOff();
        } else {
            SOCKHAND::Relays[2].removeForce();
        }

        written = snprintf(buffer, size, reply, 1, "Re3 set", data.suppData);
        break;

        case CMDS::RELAY_4: // inRange not required here due to else block.
        static uint8_t IDR4 = SOCKHAND::Relays[3].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[3].off(IDR4);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[3].on(IDR4);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[3].forceOff();
        } else {
            SOCKHAND::Relays[3].removeForce();
        }

        written = snprintf(buffer, size, reply, 1, "Re4 set", data.suppData);

        break;

        case CMDS::RELAY_1_TIMER_ON:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re1 timer bust", 0);
        } else {
            SOCKHAND::Relays[0].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re1 timer on time set", data.suppData);
        }

        break;

        case CMDS::RELAY_1_TIMER_OFF:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re1 timer bust", 0);
        } else {
            SOCKHAND::Relays[0].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re1 timer off time set", data.suppData);
        }

        break;

        case CMDS::RELAY_2_TIMER_ON:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re2 timer bust", 0);
        } else {
            SOCKHAND::Relays[1].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re2 timer on time set", data.suppData);
        }

        break;

        case CMDS::RELAY_2_TIMER_OFF:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re2 timer bust", 0);
        } else {
            SOCKHAND::Relays[1].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re2 timer off time set", data.suppData);
        }

        break;

        case CMDS::RELAY_3_TIMER_ON:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re3 timer bust", 0);
        } else {
            SOCKHAND::Relays[2].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re3 timer on time set", data.suppData);
        }

        break;

        case CMDS::RELAY_3_TIMER_OFF:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re3 timer bust", 0);
        } else {
            SOCKHAND::Relays[2].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re3 timer off time set", data.suppData);
        }

        break;

        case CMDS::RELAY_4_TIMER_ON:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re4 timer bust", 0);
        } else {
            SOCKHAND::Relays[3].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re4 timer on time set", data.suppData);
        }

        break;

        case CMDS::RELAY_4_TIMER_OFF:
        if (!SOCKHAND::inRange(0,86399, data.suppData)) { // seconds per day
            written = snprintf(buffer, size, reply, 0, "Re4 timer bust", 0);
        } else {
            SOCKHAND::Relays[3].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
            "Re4 timer off time set", data.suppData);
        }

        break;

        case CMDS::ATTACH_TEMP_RELAY: 
        if (!SOCKHAND::inRange(0, 4, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Temp Re att fail", 0);
        } else { // attach relay if within range.
            SOCKHAND::attachRelayTH( // If 4 will detach.
            data.suppData, Peripheral::TempHum::get()->getTempConf()
            );

            written = snprintf(buffer, size, reply, 1, "Temp Re att", 0);
        }

        break;

        case CMDS::SET_TEMP_LWR_THAN: 
        if (!SOCKHAND::inRange(-39, 124, data.suppData)) { // SHT limits in C
            written = snprintf(buffer, size, reply, 0, "Temp Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();
            
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->tripValRelay = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Temp <", data.suppData);
        }

        break;

        case CMDS::SET_TEMP_GTR_THAN:
        if (!SOCKHAND::inRange(-39, 124, data.suppData)) { // SHT limits in C
            written = snprintf(buffer, size, reply, 0, "Temp Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();
            
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->tripValRelay = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Temp >", data.suppData);
        }
            
        break;

        case CMDS::SET_TEMP_COND_NONE: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay if energized.
        if (conf->relay != nullptr) {
            conf->relay->off(conf->relayControlID);
        } 

        conf->condition = Peripheral::CONDITION::NONE;
        conf->tripValRelay = 0;
        written = snprintf(buffer, size, reply, 1, "Temp condition NONE", 0);
        }    

        break;

        case CMDS::ENABLE_TEMP_ALERT: 
        if (!SOCKHAND::inRange(-39, 124, data.suppData)) { // SHT limits in C
            written = snprintf(buffer, size, reply, 0, "Temp Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();
            
            conf->tripValAlert = data.suppData;
            conf->alertsEn = true;

            written = snprintf(buffer, size, reply, 1, "Temp alert EN", 
                               data.suppData);
        }

        break;

        case CMDS::DISABLE_TEMP_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        conf->tripValAlert = 0; // zero out.
        conf->alertsEn = false;
        written = snprintf(buffer, size, reply, 1, "Temp alert DEN", 0);
        }    
        
        break;

        case CMDS::ATTACH_HUM_RELAY:
        if (!SOCKHAND::inRange(0, 4, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Re att fail", 0);
        } else { // attach relay if within range.
            SOCKHAND::attachRelayTH( // If 4 will detach.
            data.suppData, Peripheral::TempHum::get()->getHumConf()
            );

            written = snprintf(buffer, size, reply, 1, "Hum Relay att", 0);
        }

        break;

        case CMDS::SET_HUM_LWR_THAN: 
        if (!SOCKHAND::inRange(1, 99, data.suppData)) { // 1 - 99% hum
            written = snprintf(buffer, size, reply, 0, "Hum Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();
            
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->tripValRelay = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Hum <", data.suppData);
        }

        break;

        case CMDS::SET_HUM_GTR_THAN: 
        if (!SOCKHAND::inRange(1, 99, data.suppData)) { // 1 - 99% hum.
            written = snprintf(buffer, size, reply, 0, "Hum Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();
            
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->tripValRelay = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Hum >", data.suppData);
        }
        
        break;

        case CMDS::SET_HUM_COND_NONE: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay.
        if (conf->relay != nullptr) {
            conf->relay->off(conf->relayControlID);
        }

        conf->condition = Peripheral::CONDITION::NONE;
        conf->tripValRelay = 0;
        written = snprintf(buffer, size, reply, 1, "Hum condition NONE", 0);
        }            
        break;

        case CMDS::ENABLE_HUM_ALERT: 
        if (!SOCKHAND::inRange(1, 99, data.suppData)) { // 1 - 99% hum.
            written = snprintf(buffer, size, reply, 0, "Temp Range Bust", 0);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();
            
            conf->tripValAlert = data.suppData;
            conf->alertsEn = true;

            written = snprintf(buffer, size, reply, 1, "Hum alert EN", 
                               data.suppData);
        }
            
        break;

        case CMDS::DISABLE_HUM_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->alertsEn = false;
        conf->tripValRelay = 0; // zero out
        written = snprintf(buffer, size, reply, 1, "Hum alert DEN", 0);
        }    
        break;

        case CMDS::SET_SOIL1_LWR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 1 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(0);
        
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so1 <", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL1_GTR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 1 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(0);
        
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so1 >", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL1_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(0);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        conf->tripValAlert = 0;
        written = snprintf(buffer, size, reply, 1, "so1 alt None", 
                           data.suppData);
        }

        break;

        case CMDS::SET_SOIL2_LWR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 2 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(1);
        
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so2 <", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL2_GTR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 2 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(1);
        
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so2 >", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL2_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(1);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        conf->tripValAlert = 0;
        written = snprintf(buffer, size, reply, 1, "so2 alt None", 
                           data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL3_LWR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 3 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(2);
        
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so3 <", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL3_GTR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 3 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(2);
        
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so3 >", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL3_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(2);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        conf->tripValAlert = 0;
        written = snprintf(buffer, size, reply, 1, "so3 alt None", 
                           data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL4_LWR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 4 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(3);
        
            conf->condition = Peripheral::CONDITION::LESS_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so4 <", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL4_GTR_THAN: 
        if (!SOCKHAND::inRange(1, 4094, data.suppData)) { // 12 bit val
            written = snprintf(buffer, size, reply, 0, "Soil 4 Range Bust", 0);
        } else {
            Peripheral::SOIL_TRIP_CONFIG* conf =
                Peripheral::Soil::get()->getConfig(3);
        
            conf->condition = Peripheral::CONDITION::GTR_THAN;
            conf->alertsEn = true;
            conf->tripValAlert = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so4 >", data.suppData);
        }
        
        break;

        case CMDS::SET_SOIL4_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(3);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        conf->tripValAlert = 0;
        written = snprintf(buffer, size, reply, 1, "so4 alt None", 
                           data.suppData);
        }
        
        break;

        case CMDS::TEST1: { // COMMENT OUT AFTER TESTING
        // Test case here
        written = snprintf(buffer, size, reply, 1, "Testing Web exchange");
        Peripheral::TempHum* th = Peripheral::TempHum::get();
        th->test(true, data.suppData);
        }
        break;

        case CMDS::TEST2: { // COMMENT OUT AFTER TESTINGS
        written = snprintf(buffer, size, reply, 1, "Testing Web exchange");
        Peripheral::TempHum* th = Peripheral::TempHum::get();
        th->test(false, data.suppData);
        }
        break;
        
        default:
        break;
    }

    if (written <= 0) {
        printf("Error formatting string\n");
    } else if (written >= size) {
        printf("Output was truncated. Buffer size: %zu, Output size: %d\n",
        size, written);
    } 
}

// Requires relay number and pointer to TempHum configuration. Ensures relay
// number is between 0 and 4, 0 - 3 (relays 1 - 4), and 4 sets the relay to
// nullptr. 
void SOCKHAND::attachRelayTH( // Temp Hum relay attach
    uint8_t relayNum, 
    Peripheral::TH_TRIP_CONFIG* conf) {
        
    if (relayNum < 4) { // Checks for valid relay number.
        // If active relay is currently assigned, ensures that it is 
        // removed from control and shut off prior to a reissue.
        if (conf->relay != nullptr) {
            conf->relay->removeID(conf->relayControlID);
        }

        conf->relay = &SOCKHAND::Relays[relayNum];
        conf->relayControlID = SOCKHAND::Relays[relayNum].getID();
        conf->relayNum = relayNum + 1; // Display purposes only

        printf("Relay %u, IDX %u, attached with ID %u\n", 
        relayNum + 1, relayNum, conf->relayControlID);

    } else if (relayNum == 4) { // 4 indicates no relay attached
        // Shuts relay off and removes its ID from array of controlling 
        // clients making it available.
        conf->relay->removeID(conf->relayControlID); 
        conf->relay = nullptr;
        conf->relayNum = 0;

        printf("Relay detached\n");
    } else {
        printf("Must be a relay number 0 - 4\n");
    }
}

// Requires lower and upper bounds, and the value. Returns true
// if the value is within the bounds. Use this for all data that
// uses data.suppData.
int SOCKHAND::inRange(int lower, int upper, int value) {
        return (value >= lower && value <= upper);
}

}