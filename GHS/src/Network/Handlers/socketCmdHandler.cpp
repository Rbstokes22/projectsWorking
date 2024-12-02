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
    
    // Relay num passed starting from index 0 for assignment.
    auto attachRelay = [](int relayNum, Peripheral::TH_TRIP_CONFIG &conf){ 
        if (relayNum >= 0 && relayNum < 4) {

            // If relay is reassigned while it is active, this will ensure that 
            // the previous relay is shut off before assigning it a new ID 
            // during attachment.
            if (conf.relay != nullptr) {
                conf.relay->off(conf.relayControlID);
            }

            conf.relay = &SOCKHAND::Relays[relayNum];
            conf.relayControlID = SOCKHAND::Relays[relayNum].getID();
            conf.relayNum = relayNum + 1; // Display purposes only
        } else if (relayNum == 4) { // 4 indicates no relay attached
            // Properly shuts relay off if detached and it is currently on.
            conf.relay->off(conf.relayControlID);
            conf.relay = nullptr;
            conf.relayNum = 0;
        } else {
            printf("Must be a relay number 0 - 4\n");
        }
    };

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
        "\"humAlertVal\":%d,\"humAlertEn\":%d,\"dHTUp\":%d,"
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

        case CMDS::CALIBRATE_TIME: {
        Clock::DateTime::get()->calibrate(data.suppData);

        written = snprintf(
            buffer, size, "Time Calibrated to %zu", 
            static_cast<size_t>(Clock::DateTime::get()->getTime()->raw)
            );
        }
        break;

        case CMDS::RELAY_1:
        static uint16_t IDR1 = SOCKHAND::Relays[0].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[0].off(IDR1);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[0].on(IDR1);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[0].forceOff();
        } else {
            SOCKHAND::Relays[0].removeForce();
        }

        written = snprintf(
            buffer, size, "Relay 1 set to %d", data.suppData
        );
        break;

        case CMDS::RELAY_2:
        static uint16_t IDR2 = SOCKHAND::Relays[1].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[1].off(IDR2);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[1].on(IDR2);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[1].forceOff();
        } else {
            SOCKHAND::Relays[1].removeForce();
        }

        written = snprintf(
            buffer, size, "Relay 2 set to %d", data.suppData
        );
        break;

        case CMDS::RELAY_3:
        static uint16_t IDR3 = SOCKHAND::Relays[2].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[2].off(IDR3);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[2].on(IDR3);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[2].forceOff();
        } else {
            SOCKHAND::Relays[2].removeForce();
        }

        written = snprintf(
            buffer, size, "Relay 3 set to %d", data.suppData
        );
        break;

        case CMDS::RELAY_4:
        static uint16_t IDR4 = SOCKHAND::Relays[3].getID(); // Perm ID
        if (data.suppData == 0) {
            SOCKHAND::Relays[3].off(IDR4);
        } else if (data.suppData == 1) {
            SOCKHAND::Relays[3].on(IDR4);
        } else if (data.suppData == 2) {
            SOCKHAND::Relays[3].forceOff();
        } else {
            SOCKHAND::Relays[3].removeForce();
        }

        written = snprintf(
            buffer, size, "Relay 4 set to %d", data.suppData
        );
        break;

        case CMDS::RELAY_1_TIMER_ON: 
        SOCKHAND::Relays[0].timerSet(true, data.suppData);
        if (data.suppData == 99999) {
            written = snprintf(buffer, size, "RE1 Timer off");
        } else {
            written = snprintf(buffer, size, "RE1 Timer on set");
        }
        break;

        case CMDS::RELAY_1_TIMER_OFF:
        SOCKHAND::Relays[0].timerSet(false, data.suppData);
        written = snprintf(buffer, size, "RE1 Timer off set");
        break;

        case CMDS::RELAY_2_TIMER_ON: 
        SOCKHAND::Relays[1].timerSet(true, data.suppData);
        if (data.suppData == 99999) {
            written = snprintf(buffer, size, "RE2 Timer off");
        } else {
            written = snprintf(buffer, size, "RE2 Timer on set");
        }
        break;

        case CMDS::RELAY_2_TIMER_OFF:
        SOCKHAND::Relays[1].timerSet(false, data.suppData);
        written = snprintf(buffer, size, "RE2 Timer off set");
        break;

        case CMDS::RELAY_3_TIMER_ON: 
        SOCKHAND::Relays[2].timerSet(true, data.suppData);
        if (data.suppData == 99999) {
            written = snprintf(buffer, size, "RE3 Timer off");
        } else {
            written = snprintf(buffer, size, "RE3 Timer on set");
        }
        break;

        case CMDS::RELAY_3_TIMER_OFF:
        SOCKHAND::Relays[2].timerSet(false, data.suppData);
        written = snprintf(buffer, size, "RE3 Timer off set");
        break;

        case CMDS::RELAY_4_TIMER_ON: 
        SOCKHAND::Relays[3].timerSet(true, data.suppData);
        if (data.suppData == 99999) {
            written = snprintf(buffer, size, "RE4 Timer off");
        } else {
            written = snprintf(buffer, size, "RE4 Timer on set");
        }
        break;

        case CMDS::RELAY_4_TIMER_OFF:
        SOCKHAND::Relays[3].timerSet(false, data.suppData);
        written = snprintf(buffer, size, "RE4 Timer off set");
        break;

        case CMDS::ATTACH_TEMP_RELAY: 
        attachRelay(data.suppData, *(Peripheral::TempHum::get()->getTempConf()));
        written = snprintf(
            buffer, size, "Relay %d attached to temp", data.suppData
            );
        break;

        case CMDS::SET_TEMP_LWR_THAN: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();
        
        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->tripValRelay = data.suppData;
        written = snprintf(buffer, size, "Temp RE < %d", data.suppData);
        }
        break;

        case CMDS::SET_TEMP_GTR_THAN: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->tripValRelay = data.suppData;
        written = snprintf(buffer, size, "Temp RE > %d", data.suppData);
        }     
        break;

        case CMDS::SET_TEMP_COND_NONE: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay.
        if (conf->relay != nullptr) {
            conf->relay->off(conf->relayControlID);
        }

        conf->condition = Peripheral::CONDITION::NONE;
        written = snprintf(buffer, size, "Temp RE NONE");
        }    
        break;

        case CMDS::ENABLE_TEMP_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();
  
        conf->tripValAlert = data.suppData;
        conf->alertsEn = true;
        written = snprintf(buffer, size, "Temp ALT en @ %d", data.suppData);
        }    
        break;

        case CMDS::DISABLE_TEMP_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        conf->alertsEn = false;
        written = snprintf(buffer, size, "Temp ALT disabled");
        }    
        break;

        case CMDS::ATTACH_HUM_RELAY:
        attachRelay(data.suppData, *(Peripheral::TempHum::get()->getHumConf())); 
        written = snprintf(
            buffer, size, "Relay %d attached to hum", data.suppData
            );
        break;

        case CMDS::SET_HUM_LWR_THAN: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->tripValRelay = data.suppData;
        written = snprintf(buffer, size, "Hum RE < %d", data.suppData);
        }    
        break;

        case CMDS::SET_HUM_GTR_THAN: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->tripValRelay = data.suppData;
        written = snprintf(buffer, size, "Hum RE > %d", data.suppData);
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
        written = snprintf(buffer, size, "Hum RE NONE");
        }            
        break;

        case CMDS::ENABLE_HUM_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->tripValAlert = data.suppData;
        conf->alertsEn = true;
        written = snprintf(buffer, size, "Hum ALT en @ %d", data.suppData);
        }      
        break;

        case CMDS::DISABLE_HUM_ALERT: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->alertsEn = false;
        conf->tripValRelay = data.suppData;
        written = snprintf(buffer, size, "Hum ALT disabled");
        }    
        break;

        case CMDS::SET_SOIL1_LWR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(0);
      
        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 1 ALT < %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL1_GTR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(0);

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 1 ALT > %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL1_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(0);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        written = snprintf(buffer, size, "Soil 1 ALT None");
        }
        break;

        case CMDS::SET_SOIL2_LWR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(1);
      
        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 2 ALT < %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL2_GTR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(1);

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 2 ALT > %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL2_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(1);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        written = snprintf(buffer, size, "Soil 2 ALT None");
        }
        break;

        case CMDS::SET_SOIL3_LWR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(2);
      
        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 3 ALT < %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL3_GTR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(2);

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 3 ALT > %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL3_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(2);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        written = snprintf(buffer, size, "Soil 3 ALT None");
        }
        break;

        case CMDS::SET_SOIL4_LWR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(3);
      
        conf->condition = Peripheral::CONDITION::LESS_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 4 ALT < %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL4_GTR_THAN: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(3);

        conf->condition = Peripheral::CONDITION::GTR_THAN;
        conf->alertsEn = true;
        conf->tripValAlert = data.suppData;
        written = snprintf(buffer, size, "Soil 4 ALT > %d", data.suppData);
        }
        break;

        case CMDS::SET_SOIL4_COND_NONE: {
        Peripheral::SOIL_TRIP_CONFIG* conf =
            Peripheral::Soil::get()->getConfig(3);

        conf->condition = Peripheral::CONDITION::NONE;
        conf->alertsEn = false;
        written = snprintf(buffer, size, "Soil 4 ALT None");
        }
        break;

        case CMDS::TEST: { // DELETE AFTER TESTING WEB SERVER EXCHANGE
        // Test case here
        written = snprintf(buffer, size, "Testing Web exchange");
        }
        

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

}