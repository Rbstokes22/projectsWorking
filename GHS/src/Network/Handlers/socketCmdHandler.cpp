#include "Network/Handlers/socketHandler.hpp"
#include "esp_http_server.h"
#include "Network/NetMain.hpp"
#include "Network/NetSTA.hpp"
#include "Threads/Mutex.hpp"
#include "Config/config.hpp"
#include "Peripherals/Relay.hpp"
#include "Peripherals/Alert.hpp"
#include "Peripherals/TempHum.hpp"
#include "Peripherals/Light.hpp"
#include "Peripherals/Soil.hpp"
#include "Peripherals/Report.hpp"
#include "Common/Timing.hpp"
#include "Drivers/SHT_Library.hpp" 
#include "UI/MsgLogHandler.hpp" 
#include "Peripherals/saveSettings.hpp" 
#include "Network/Handlers/MasterHandler.hpp"

// NOTE. Some of these functionalities are for station mode only. These commands
// have a built in check to ensure requirements are met.

namespace Comms {

// Requires command data, buffer, and buffer size. Executes the command passed,
// and compiles response into json and replies.
void SOCKHAND::compileData(cmdData &data, char* buffer, size_t size) {
    int written{-1}; // Ensures the snprintf is working by checking write size.
    bool writeLog = true; // Set to false by functions not meant to log.

    // reply is the typical reply to a request, with the exception of the
    // GET_ALL request. Used throughout the function.
    const char* reply = "{\"status\":%d,\"msg\":\"%s\",\"supp\":%d,"
    "\"id\":\"%s\"}"; 

    // All commands work from here. CMD list is on header doc.
    switch(data.cmd) {

        // Gets all sensor and some system data and sends JSON back to client.
        // This is the reason the buffer is large, due to the large amount
        // of data required to pass to the client.
        case CMDS::GET_ALL: {
        
        writeLog = false; // Commonly used command, do not want to log.

        // Commonly used pointers in the scope of GET_ALL. Declared them here
        // to avoid using verbose commands.
        Clock::DateTime* dtg = Clock::DateTime::get();
        Peripheral::TempHum* th = Peripheral::TempHum::get();
        Peripheral::Timer* re1Timer = SOCKHAND::Relays[0].getTimer();
        Peripheral::Timer* re2Timer = SOCKHAND::Relays[1].getTimer();
        Peripheral::Timer* re3Timer = SOCKHAND::Relays[2].getTimer();
        Peripheral::Timer* re4Timer = SOCKHAND::Relays[3].getTimer();
        Peripheral::Soil* soil = Peripheral::Soil::get();
        Peripheral::Light* light = Peripheral::Light::get();
        
        // Primary JSON response object. Populates every setting and value that
        // is critical to the operation of this device, and returns it to the
        // client. Ensure client uses same JSON and same commands.
        written = snprintf(buffer, size,  
        "{\"firmv\":\"%s\",\"id\":\"%s\",\"newLog\":%d,"
        "\"sysTime\":%zu,\"hhmmss\":\"%d:%d:%d\",\"timeCalib\":%d,"
        "\"re1\":%d,\"re1TimerEn\":%d,\"re1TimerOn\":%zu,\"re1TimerOff\":%zu,"
        "\"re2\":%d,\"re2TimerEn\":%d,\"re2TimerOn\":%zu,\"re2TimerOff\":%zu,"
        "\"re3\":%d,\"re3TimerEn\":%d,\"re3TimerOn\":%zu,\"re3TimerOff\":%zu,"
        "\"re4\":%d,\"re4TimerEn\":%d,\"re4TimerOn\":%zu,\"re4TimerOff\":%zu,"
        "\"temp\":%.2f,\"tempRe\":%d,\"tempReCond\":%u,\"tempReVal\":%d,"
        "\"tempAltCond\":%u,\"tempAltVal\":%d,"
        "\"hum\":%.2f,\"humRe\":%d,\"humReCond\":%u,\"humReVal\":%d,"
        "\"humAltCond\":%u,\"humAltVal\":%d,"
        "\"SHTUp\":%d,\"tempAvg\":%0.2f,\"humAvg\":%0.2f,"
        "\"tempAvgPrev\":%0.2f,\"humAvgPrev\":%0.2f,"
        "\"soil1\":%d,\"soil1Cond\":%u,\"soil1AltVal\":%d,\"soil1Up\":%d,"
        "\"soil2\":%d,\"soil2Cond\":%u,\"soil2AltVal\":%d,\"soil2Up\":%d,"
        "\"soil3\":%d,\"soil3Cond\":%u,\"soil3AltVal\":%d,\"soil3Up\":%d,"
        "\"soil4\":%d,\"soil4Cond\":%u,\"soil4AltVal\":%d,\"soil4Up\":%d,"
        "\"violet\":%u,\"indigo\":%u,\"blue\":%u,\"cyan\":%u,\"green\":%u,"
        "\"yellow\":%u,\"orange\":%u,\"red\":%u,\"nir\":%u,\"clear\":%u,"
        "\"photo\":%d,"
        "\"violetAvg\":%0.2f,\"indigoAvg\":%0.2f,\"blueAvg\":%0.2f,"
        "\"cyanAvg\":%0.2f,\"greenAvg\":%0.2f,\"yellowAvg\":%0.2f,"
        "\"orangeAvg\":%0.2f,\"redAvg\":%0.2f,\"nirAvg\":%0.2f,"
        "\"clearAvg\":%0.2f,\"photoAvg\":%0.2f,"
        "\"violetAvgPrev\":%0.2f,\"indigoAvgPrev\":%0.2f,\"blueAvgPrev\":%0.2f,"
        "\"cyanAvgPrev\":%0.2f,\"greenAvgPrev\":%0.2f,\"yellowAvgPrev\":%0.2f,"
        "\"orangeAvgPrev\":%0.2f,\"redAvgPrev\":%0.2f,\"nirAvgPrev\":%0.2f,"
        "\"clearAvgPrev\":%0.2f,\"photoAvgPrev\":%0.2f,"
        "\"lightRe\":%d,\"lightReCond\":%u,\"lightReVal\":%u,"
        "\"lightDur\":%lu,\"photoUp\":%d,\"specUp\":%d,\"darkVal\":%u,"
        "\"repTimeEn\":%d,\"repSendTime\":%lu}",
        FIRMWARE_VERSION, 
        data.idNum,
        Messaging::MsgLogHandler::get()->newLogAvail(),
        static_cast<size_t>(dtg->getTime()->raw), 
        dtg->getTime()->hour, dtg->getTime()->minute, dtg->getTime()->second,
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
        th->getTempConf()->relay.num,
        static_cast<uint8_t>(th->getTempConf()->relay.condition),
        th->getTempConf()->relay.tripVal,
        static_cast<uint8_t>(th->getTempConf()->alt.condition),
        th->getTempConf()->alt.tripVal,
        th->getHum(),
        th->getHumConf()->relay.num,
        static_cast<uint8_t>(th->getHumConf()->relay.condition),
        th->getHumConf()->relay.tripVal,
        static_cast<uint8_t>(th->getHumConf()->alt.condition),
        th->getHumConf()->alt.tripVal,
        th->getStatus().noDispErr,
        th->getAverages()->temp,
        th->getAverages()->hum,
        th->getAverages()->prevTemp,
        th->getAverages()->prevHum,
        soil->getReadings(0)->val, 
        static_cast<uint8_t>(soil->getConfig(0)->condition),
        soil->getConfig(0)->tripVal, soil->getReadings(0)->noDispErr,
        soil->getReadings(1)->val, 
        static_cast<uint8_t>(soil->getConfig(1)->condition),
        soil->getConfig(1)->tripVal, soil->getReadings(1)->noDispErr,
        soil->getReadings(2)->val, 
        static_cast<uint8_t>(soil->getConfig(2)->condition),
        soil->getConfig(2)->tripVal, soil->getReadings(2)->noDispErr,
        soil->getReadings(3)->val, 
        static_cast<uint8_t>(soil->getConfig(3)->condition),
        soil->getConfig(3)->tripVal, soil->getReadings(3)->noDispErr,
        light->getSpectrum()->F1_415nm_Violet,
        light->getSpectrum()->F2_445nm_Indigo,
        light->getSpectrum()->F3_480nm_Blue,
        light->getSpectrum()->F4_515nm_Cyan,
        light->getSpectrum()->F5_555nm_Green,
        light->getSpectrum()->F6_590nm_Yellow,
        light->getSpectrum()->F7_630nm_Orange,
        light->getSpectrum()->F8_680nm_Red,
        light->getSpectrum()->NIR,
        light->getSpectrum()->Clear,
        light->getPhoto(),
        light->getAverages()->color.violet,
        light->getAverages()->color.indigo,
        light->getAverages()->color.blue,
        light->getAverages()->color.cyan,
        light->getAverages()->color.green,
        light->getAverages()->color.yellow,
        light->getAverages()->color.orange,
        light->getAverages()->color.red,
        light->getAverages()->color.nir,
        light->getAverages()->color.clear,
        light->getAverages()->photoResistor,
        light->getAverages()->prevColor.violet,
        light->getAverages()->prevColor.indigo,
        light->getAverages()->prevColor.blue,
        light->getAverages()->prevColor.cyan,
        light->getAverages()->prevColor.green,
        light->getAverages()->prevColor.yellow,
        light->getAverages()->prevColor.orange,
        light->getAverages()->prevColor.red,
        light->getAverages()->prevColor.nir,
        light->getAverages()->prevColor.clear,
        light->getAverages()->prevPhotoResistor,
        light->getConf()->num,
        static_cast<uint8_t>(light->getConf()->condition),
        light->getConf()->tripVal,
        light->getDuration(),
        light->getStatus().photoNoDispErr,
        light->getStatus().specNoDispErr,
        light->getConf()->darkVal,
        Peripheral::Report::get()->getTimeData()->isSet,
        Peripheral::Report::get()->getTimeData()->timeSet
        );
        }
        break;

        // Calibrates the system time. There are 86400 seconds per day, and
        // the client will use this command and pass the current seconds past
        // midnight. The system time will then be set to the client time. This
        // is to ensure that error accumulation doesnt occur, and that time
        // dependent functions can run.
        case CMDS::CALIBRATE_TIME: 
        if (!SOCKHAND::inRange(0, 86399, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, 
                "Time out of range", data.suppData, data.idNum);
        } else {
            Clock::DateTime::get()->calibrate(data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Time calibrated to", data.suppData, data.idNum);
        }
        
        break;

        // If a new log entry is available, in the GET_ALL socket command,
        // there will be a flag showing 1. Once the client acknowledges and
        // receives the log, they will send a socket command here which will
        // reset the flag back to false.
        case CMDS::NEW_LOG_RCVD: {

            writeLog = false; // Not req to log a log.

            Messaging::MsgLogHandler::get()->resetNewLogFlag();

            written = snprintf(buffer, size, reply, 1,
                "Log Rcvd", 0, data.idNum);
        }

        break;

        // Relays 1 - 4 controls. Changes relay status to supp data which will
        // control the relay behavior to on, off, forced off, and force off 
        // removed. Forced off is an override feature that will shut the relay
        // off despite another function/process using it.
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

        written = snprintf(buffer, size, reply, 1, "Re1 set", data.suppData,
            data.idNum);

        break;

        // See RELAY_1
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
        
        written = snprintf(buffer, size, reply, 1, "Re2 set", data.suppData, 
            data.idNum);

        break;

        // See RELAY_1
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

        written = snprintf(buffer, size, reply, 1, "Re3 set", data.suppData, 
            data.idNum);

        break;

        // See RELAY_1
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

        written = snprintf(buffer, size, reply, 1, "Re4 set", data.suppData, 
            data.idNum);

        break;

        // Sets the relay 1 timer on time. A value not exceeding 86399 must 
        // be passed and the timer will be set to turn on when the system time
        // reaches that value in seconds. If the value 99999 is passed for 
        // either timer on or off, it will shut the 
        
        //APPLIES FOR RELAYS 1 - 4
        case CMDS::RELAY_1_TIMER_ON:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re1 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[0].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re1 timer on time set", data.suppData, data.idNum);
        }

        break;

        // Sets the relay 1 timer off time. A value not exceeding 86399 must 
        // be passed and the timer will be set to turn off when the system time
        // reaches that value in seconds. APPLIES FOR RELAYS 1 - 4
        case CMDS::RELAY_1_TIMER_OFF:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re1 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[0].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re1 timer off time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_TIMER_ON
        case CMDS::RELAY_2_TIMER_ON:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re2 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[1].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re2 timer on time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_IMER_OFF
        case CMDS::RELAY_2_TIMER_OFF:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re2 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[1].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re2 timer off time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_TIMER_ON
        case CMDS::RELAY_3_TIMER_ON:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re3 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[2].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re3 timer on time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_IMER_OFF
        case CMDS::RELAY_3_TIMER_OFF:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) {
            written = snprintf(buffer, size, reply, 0, "Re3 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[2].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re3 timer off time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_TIMER_ON
        case CMDS::RELAY_4_TIMER_ON:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re4 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[3].timerSet(true, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re4 timer on time set", data.suppData, data.idNum);
        }

        break;

        // See RELAY_1_IMER_OFF
        case CMDS::RELAY_4_TIMER_OFF:
        if (!SOCKHAND::inRange(0, 86399, data.suppData, RELAY_TIMER_OFF)) { 
            written = snprintf(buffer, size, reply, 0, "Re4 timer bust", 0, 
                data.idNum);
        } else {
            SOCKHAND::Relays[3].timerSet(false, data.suppData);
            written = snprintf(buffer, size, reply, 1, 
                "Re4 timer off time set", data.suppData, data.idNum);
        }

        break;

        // Attaches a single relay to the temperature sensor. Supp data should
        // be 0 to 3, corresponding to relays 1 - 4, or 4, which will detach 
        // the relay.
        case CMDS::ATTACH_TEMP_RELAY: 
        if (!SOCKHAND::inRange(0, 4, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Temp Re att fail", 0, 
                data.idNum);
        } else { // attach relay if within range.

            writeLog = false; // Has logging.
            SOCKHAND::attachRelayTH( // If 4 will detach.
                data.suppData, Peripheral::TempHum::get()->getTempConf()
            );

            written = snprintf(buffer, size, reply, 1, "Temp Re att", 
                data.suppData, data.idNum);
        }

        break;

        // Sets temperature relay to turn on if lower than that supp data 
        // passed. Supp data will be in celcius to function correctly. If 20
        // is passed, then relay will turn on when the temperature is lower 
        // than 20. 
        case CMDS::SET_TEMP_RE_LWR_THAN: 
        if (!SOCKHAND::inRange(SHT_MIN, SHT_MAX, data.suppData)) { // SHT limits in C
            written = snprintf(buffer, size, reply, 0, "Temp RE RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf(); 
            
            conf->relay.condition = Peripheral::RECOND::LESS_THAN;
            conf->relay.tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Temp Re <", 
                data.suppData, data.idNum);
        }

        break;

        // Sets temperature relay to turn on if greater than that supp data 
        // passed. Supp data will be in celcius to function correctly. If 20
        // is passed, then relay will turn on when the temperature is greater 
        // than 20. 
        case CMDS::SET_TEMP_RE_GTR_THAN:
        if (!SOCKHAND::inRange(SHT_MIN, SHT_MAX, data.suppData)) { // SHT limits in C
            written = snprintf(buffer, size, reply, 0, "Temp Re RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();
            
            conf->relay.condition = Peripheral::RECOND::GTR_THAN;
            conf->relay.tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Temp RE >", 
                data.suppData, data.idNum);
        }
            
        break;

        // Removes the lower or greater than condition. This will also ensure 
        // that the attached relay will turn off if one is currently attached,
        // and the trip value will be set to 0.
        case CMDS::SET_TEMP_RE_COND_NONE: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay if energized.
        if (conf->relay.relay != nullptr) {
            conf->relay.relay->off(conf->relay.controlID); 
        } 

        conf->relay.condition = Peripheral::RECOND::NONE;
        conf->relay.tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "Temp Re cond NONE", 0, 
            data.idNum);
        }    

        break;

        // Sets temperature alert to send if lower than that supp data 
        // passed. Supp data will be in celcius to function correctly. If 20
        // is passed, then alert will send when the temperature is lower 
        // than 20. 
        case CMDS::SET_TEMP_ALT_LWR_THAN:

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SHT_MIN, SHT_MAX, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Temp Alt RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();

            conf->alt.condition = Peripheral::ALTCOND::LESS_THAN;
            conf->alt.tripVal = data.suppData;
            written = snprintf(buffer, size, reply, 1, "Temp Alt <", 
                data.suppData, data.idNum);
        }

        break;

        // Sets temperature alert to send if greater than that supp data 
        // passed. Supp data will be in celcius to function correctly. If 20
        // is passed, then alert will send when the temperature is greater 
        // than 20. 
        case CMDS::SET_TEMP_ALT_GTR_THAN:

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SHT_MIN, SHT_MAX, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Temp Alt RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getTempConf();

            conf->alt.condition = Peripheral::ALTCOND::GTR_THAN;
            conf->alt.tripVal = data.suppData;
            written = snprintf(buffer, size, reply, 1, "Temp Alt >", 
                data.suppData, data.idNum);
        }

        break;

        // Removes the lower or greater than condition. This will also ensure 
        // that the trip value will be set to 0.
        case CMDS::SET_TEMP_ALT_COND_NONE: {
        
        // STATION CHECK NOT NEEDED.
        
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getTempConf();

        conf->alt.condition = Peripheral::ALTCOND::NONE;
        conf->alt.tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "Temp Alt cond NONE", 0, 
            data.idNum);
        }   

        break;

        // ALL COMMANDS ABOVE REGARDING TEMPERATURE ARE COPIES FOR ALL 
        // COMMANDS REGARDING HUMITIDY BELOW FROM ATTACH_HUM_RELAY TO 
        // SET_HUM_ALT_COND_NONE. 0 to 4 must be passed.
        case CMDS::ATTACH_HUM_RELAY:
        if (!SOCKHAND::inRange(0, 4, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Re att fail", 0, 
                data.idNum);
        } else { // attach relay if within range.

            writeLog = false; // Has logging.
            SOCKHAND::attachRelayTH( // If 4 will detach.
                data.suppData, Peripheral::TempHum::get()->getHumConf()
            );

            written = snprintf(buffer, size, reply, 1, "Hum Relay att", 
                data.suppData, data.idNum);
        }

        break;

        // See SET_TEMP_RE_LWR_THAN.
        case CMDS::SET_HUM_RE_LWR_THAN: 
        if (!SOCKHAND::inRange(SHT_MIN_HUM, SHT_MAX_HUM, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();
            
            conf->relay.condition = Peripheral::RECOND::LESS_THAN;
            conf->relay.tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Hum <", data.suppData, 
                data.idNum);
        }

        break;

        // See SET_TEMP_RE_GTR_THAN.
        case CMDS::SET_HUM_RE_GTR_THAN: 
        if (!SOCKHAND::inRange(SHT_MIN_HUM, SHT_MAX_HUM, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();
            
            conf->relay.condition = Peripheral::RECOND::GTR_THAN;
            conf->relay.tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Hum >", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_TEMP_RE_COND_NONE.
        case CMDS::SET_HUM_RE_COND_NONE: {
        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay.
        if (conf->relay.relay != nullptr) {
            conf->relay.relay->off(conf->relay.controlID);
        }

        conf->relay.condition = Peripheral::RECOND::NONE;
        conf->relay.tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "Hum condition NONE", 0, 
            data.idNum);
        }            
        break;

        // See SET_TEMP_ALT_LWR_THAN.
        case CMDS::SET_HUM_ALT_LWR_THAN:

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SHT_MIN_HUM, SHT_MAX_HUM, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Alt RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();

            conf->alt.condition = Peripheral::ALTCOND::LESS_THAN;
            conf->alt.tripVal = data.suppData;
            written = snprintf(buffer, size, reply, 1, "Hum Alt <", 
                data.suppData, data.idNum);
        }

        break;

        // See SET_TEMP_ALT_GTR_THAN.
        case CMDS::SET_HUM_ALT_GTR_THAN:

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SHT_MIN_HUM, SHT_MAX_HUM, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Hum Alt RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::TH_TRIP_CONFIG* conf = 
                Peripheral::TempHum::get()->getHumConf();

            conf->alt.condition = Peripheral::ALTCOND::GTR_THAN;
            conf->alt.tripVal = data.suppData;
            written = snprintf(buffer, size, reply, 1, "Hum Alt >", 
                data.suppData, data.idNum);
        }

        break;

        // See SET_TEMP_ALT_COND_NONE.
        case CMDS::SET_HUM_ALT_COND_NONE: {

        // STATION CHECK NOT NEEDED.

        Peripheral::TH_TRIP_CONFIG* conf = 
            Peripheral::TempHum::get()->getHumConf();

        conf->alt.condition = Peripheral::ALTCOND::NONE;
        conf->alt.tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "Hum Alt cond NONE", 0, 
            data.idNum);
        }   

        break;

        // Clears the temperature and humidity average values. 
        case CMDS::CLEAR_TEMPHUM_AVG: {
            Peripheral::TempHum::get()->clearAverages();
            written = snprintf(buffer, size, reply, 1, "TH Avg Clear", 0, 
                data.idNum);
        }

        break;

        // Sets soil 1 alert to send if lower than that supp data passed.
        // Supp data will be between 1 and 4094, and is an analog reading of
        // the capacitance of the soil. There is no calibration, so each sensor
        // might have unique readings that the client will be able to adjust
        // to. If 2000 is passed, an alert will send when the soil reading is
        // below 2000. APPLIES TO SOIL 1 - 4.
        case CMDS::SET_SOIL1_LWR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 1 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(0);
        
            conf->condition = Peripheral::ALTCOND::LESS_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so1 <", data.suppData, 
                data.idNum);
        }
        
        break;

        // Sets soil 1 alert to send if greater than that supp data passed.
        // Supp data will be between 1 and 4094, and is an analog reading of
        // the capacitance of the soil. There is no calibration, so each sensor
        // might have unique readings that the client will be able to adjust
        // to. If 2000 is passed, an alert will send when the soil reading is
        // above 2000. APPLIES TO SOIL 1 - 4.
        case CMDS::SET_SOIL1_GTR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 1 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(0);
        
            conf->condition = Peripheral::ALTCOND::GTR_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so1 >", data.suppData, 
                data.idNum);
        }
        
        break;

        // Removes the greater/lower than condition and resets the trip value
        // alert to 0, and disables alerts. APPLIES TO SOIL 1 - 4.
        case CMDS::SET_SOIL1_COND_NONE: {

        // STATION CHECK NOT NEEDED.

        Peripheral::AlertConfigSo* conf =
            Peripheral::Soil::get()->getConfig(0);

        conf->condition = Peripheral::ALTCOND::NONE;
        conf->tripVal = 0;

        written = snprintf(buffer, size, reply, 1, "so1 alt None", 
            data.suppData, data.idNum);
        }

        break;

        // See SET_SOIL1_LWR_THAN.
        case CMDS::SET_SOIL2_LWR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 2 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(1);
        
            conf->condition = Peripheral::ALTCOND::LESS_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so2 <", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_GTR_THAN.
        case CMDS::SET_SOIL2_GTR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 2 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(1);
        
            conf->condition = Peripheral::ALTCOND::GTR_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so2 >", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_COND_NONE.
        case CMDS::SET_SOIL2_COND_NONE: {

        // STATION CHECK NOT NEEDED.

        Peripheral::AlertConfigSo* conf =
            Peripheral::Soil::get()->getConfig(1);

        conf->condition = Peripheral::ALTCOND::NONE;
        conf->tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "so2 alt None", 
            data.suppData, data.idNum);
        }
        
        break;

        // See SET_SOIL1_LWR_THAN.
        case CMDS::SET_SOIL3_LWR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Soil 3 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(2);
        
            conf->condition = Peripheral::ALTCOND::LESS_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so3 <", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_GTR_THAN.
        case CMDS::SET_SOIL3_GTR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 3 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(2);
        
            conf->condition = Peripheral::ALTCOND::GTR_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so3 >", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_COND_NONE.
        case CMDS::SET_SOIL3_COND_NONE: {

        // STATION CHECK NOT NEEDED.

        Peripheral::AlertConfigSo* conf =
            Peripheral::Soil::get()->getConfig(2);

        conf->condition = Peripheral::ALTCOND::NONE;
        conf->tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "so3 alt None", 
            data.suppData, data.idNum);
        }
        
        break;

        // See SET_SOIL1_LWR_THAN.
        case CMDS::SET_SOIL4_LWR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 4 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(3);
        
            conf->condition = Peripheral::ALTCOND::LESS_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so4 <", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_GTR_THAN.
        case CMDS::SET_SOIL4_GTR_THAN: 

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Soil 4 Range Bust", 0, 
                data.idNum);
        } else {
            Peripheral::AlertConfigSo* conf =
                Peripheral::Soil::get()->getConfig(3);
        
            conf->condition = Peripheral::ALTCOND::GTR_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "so4 >", data.suppData, 
                data.idNum);
        }
        
        break;

        // See SET_SOIL1_COND_NONE.
        case CMDS::SET_SOIL4_COND_NONE: {

        // STATION CHECK NOT NEEDED.

        Peripheral::AlertConfigSo* conf =
            Peripheral::Soil::get()->getConfig(3);

        conf->condition = Peripheral::ALTCOND::NONE;
        conf->tripVal = 0;
        written = snprintf(buffer, size, reply, 1, "so4 alt None", 
            data.suppData, data.idNum);
        }
        
        break;

        // Attaches a single relay to the photoresistor. Supp data should
        // be 0 to 3, corresponding to relays 1 - 4, or 4, which will detach 
        // the relay. AS7341 is read only.
        case CMDS::ATTACH_LIGHT_RELAY:
        if (!SOCKHAND::inRange(0, 4, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Lgt Re att fail", 0, 
                data.idNum);
        } else { // attach relay if within range.

            writeLog = false; // Has logging.
            SOCKHAND::attachRelayLT( // If 4 will detach.
                data.suppData, Peripheral::Light::get()->getConf()
            );

            written = snprintf(buffer, size, reply, 1, "Lgt Re att", 
                data.suppData, data.idNum);
        }

        break;

        // Sets the light relay to turn on if lower than the supp data passed.
        // Supp data will be between 1 & 4094 (12-bit). If 500 is passed
        // then the relay will turn on when the photoresistor reading is below
        // 500.
        case CMDS::SET_LIGHT_RE_LWR_THAN: 
        if (!SOCKHAND::inRange(PHOTO_MIN, PHOTO_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Lgt RE RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::RelayConfigLight* conf =
                Peripheral::Light::get()->getConf();

            conf->condition = Peripheral::RECOND::LESS_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Lgt Re <", 
                data.suppData, data.idNum);
        }

        break;

        // Sets the light relay to turn on if greater than the supp data passed.
        // Supp data will be between 1 & 4094 (12-bit). If 500 is passed
        // then the relay will turn on when the photoresistor reading is above
        // 500.
        case CMDS::SET_LIGHT_RE_GTR_THAN:
        if (!SOCKHAND::inRange(PHOTO_MIN, PHOTO_MAX, data.suppData)) { 
            written = snprintf(buffer, size, reply, 0, "Lgt RE RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::RelayConfigLight* conf =
                Peripheral::Light::get()->getConf();

            conf->condition = Peripheral::RECOND::GTR_THAN;
            conf->tripVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Lgt Re >", 
                data.suppData, data.idNum);
        }

        break;

        // Removes the lower or greater than condition. This will also ensure
        // that the attached relay will turn off if one is currently attached.
        // The trip value will then be set to 0.
        case CMDS::SET_LIGHT_RE_COND_NONE: {
        Peripheral::RelayConfigLight* conf =
            Peripheral::Light::get()->getConf();

        // If relay is active, switching to condtion NONE will 
        // shut off the relay if energized.
        if (conf->relay != nullptr) {
            conf->relay->off(conf->controlID); 
        } 

        conf->condition = Peripheral::RECOND::NONE;
        conf->tripVal = 0;

        written = snprintf(buffer, size, reply, 1, "Lgt Re cond NONE", 0,
            data.idNum);
        }

        break;

        // Sets the dark value of the photoresitor. This allows daylight
        // duration to be counted.
        case CMDS::SET_DARK_VALUE: 
        if (!SOCKHAND::inRange(PHOTO_MIN, PHOTO_MAX, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Lgt Dark RangeErr", 0, 
                data.idNum);
        } else {
            Peripheral::Light::get()->getConf()->darkVal = data.suppData;

            written = snprintf(buffer, size, reply, 1, "Dark Val", 
                data.suppData, data.idNum);
        }

        break;

        // Clears the light average values. 
        case CMDS::CLEAR_LIGHT_AVG:
        Peripheral::Light::get()->clearAverages();
        written = snprintf(buffer, size, reply, 1, "Lgt Avg Clear", 0, 
                data.idNum);

        break;

        // // Sets the second past midnight that the daily report is set to send.
        // // IF value 99999 is passed, it will shut the timer down until reset
        // // with a proper integer.
        case CMDS::SEND_REPORT_SET_TIME:

        // Prevents WAP mode from running station only features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {}

        else if (!SOCKHAND::inRange(0, MAX_SET_TIME, data.suppData, 
            TIMER_OFF)) {

            written = snprintf(buffer, size, reply, 0, "Report timer bust", 0, 
                data.idNum);
        } else {
            Peripheral::Report::get()->setTimer(data.suppData);

            written = snprintf(buffer, size, reply, 1, "Report Time set", 
            data.suppData, data.idNum);
        }

        break;

        // When called, the device will save all configuration settings to the
        // NVS and restart the system.
        case CMDS::SAVE_AND_RESTART:
        NVS::settingSaver::get()->save();
        esp_restart();
        break;

        default:
        break;
    }

    if (writeLog) { // Will log the JSON response string.
        MASTERHAND::sendErr(buffer, Messaging::Levels::INFO);
    }

    if (written <= 0) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s error formatting string", SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);

    } else if (written >= size) {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Output truncated. Buffer size: %zu, Output size: %d", 
            SOCKHAND::tag, size, written);

        MASTERHAND::sendErr(MASTERHAND::log);
    } 
}

// Requires the lower and upper bounds, the value, and the exception value.
// The exception value is if a value is out of range, but equals the exception,
// it continues. An example is if you have the seconds of the day for a timer,
// it can look like inRange(0, 86399, 4252, 99999), which will determine if 
// 4252 is between 0 and 86399, or if it equals 99999. The default exception
// is set to -999. Returns true if good, or false if not.
bool SOCKHAND::inRange(int lower, int upper, int value, int exception) {
        return ((value >= lower && value <= upper) || (value == exception));
}

// Requires the written int ref, buffer, buffer size, reply char array, and
// id number. Checks to ensure that the mode is in station mode, because only
// certain features are available in that mode. If yes, returns true, if not
// writes reply, and returns false.
bool SOCKHAND::checkSTA(int &written, char* buffer, size_t size, 
    const char* reply, const char* idNum) {

    if (NetMain::getNetType() != NetMode::STA) {
        written = snprintf(buffer, size, reply, 0, "ONLY STA MODE ALLOWED", 
            0, idNum);

        return false; // Is not station mode.
    }

    return true; // is station mode.
}

// Requires relay number and pointer to TempHum configuration. Ensures relay
// number is between 0 and 4, 0 - 3 (relays 1 - 4), and 4 sets the relay to
// nullptr and remove functionality. 
void SOCKHAND::attachRelayTH(uint8_t relayNum, 
    Peripheral::TH_TRIP_CONFIG* conf) {
        
    if (relayNum < 4) { // Checks for valid relay number.
        // If active relay is currently assigned, ensures that it is 
        // removed from control and shut off prior to a reissue.
        if (conf->relay.relay != nullptr) {
            conf->relay.relay->removeID(conf->relay.controlID);
        }

        conf->relay.relay = &SOCKHAND::Relays[relayNum];
        conf->relay.controlID = SOCKHAND::Relays[relayNum].getID();
        conf->relay.num = relayNum; // Display purposes only

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Relay %u, IDX %u, attached with ID %u", 
            SOCKHAND::tag, relayNum + 1, relayNum, conf->relay.controlID);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

    } else if (relayNum == 4) { // 4 indicates no relay attached
        // Shuts relay off and removes its ID from array of controlling 
        // clients making it available.
        conf->relay.relay->removeID(conf->relay.controlID); 
        conf->relay.relay = nullptr;
        conf->relay.num = TEMP_HUM_NO_RELAY;

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Relay ID %u detached", SOCKHAND::tag, conf->relay.controlID);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

    } else {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s TH relay not attached, incorrect index <%u> passed", 
            SOCKHAND::tag, relayNum);
        
        MASTERHAND::sendErr(MASTERHAND::log);
    }
}

void SOCKHAND::attachRelayLT(uint8_t relayNum, 
    Peripheral::RelayConfigLight* conf) {

    if (relayNum < 4) { // Checks for valid relay number.
        // If active relay is currently assigned, ensures that it is 
        // removed from control and shut off prior to a reissue.
        if (conf->relay != nullptr) {
            conf->relay->removeID(conf->controlID);
        }

        conf->relay = &SOCKHAND::Relays[relayNum];
        conf->controlID = SOCKHAND::Relays[relayNum].getID();
        conf->num = relayNum; // Display purposes only

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Relay %u, IDX %u, attached with ID %u", 
            SOCKHAND::tag, relayNum + 1, relayNum, conf->controlID);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

    } else if (relayNum == 4) { // 4 indicates no relay attached
        // Shuts relay off and removes its ID from array of controlling 
        // clients making it available.
        conf->relay->removeID(conf->controlID); 
        conf->relay = nullptr;
        conf->num = LIGHT_NO_RELAY;

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Relay ID %u detached", SOCKHAND::tag, conf->controlID);

        MASTERHAND::sendErr(MASTERHAND::log, Messaging::Levels::INFO);

    } else {

        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s Light relay not attached, incorrect index <%u> passed", 
            SOCKHAND::tag, relayNum);

        MASTERHAND::sendErr(MASTERHAND::log);
    }
}

}