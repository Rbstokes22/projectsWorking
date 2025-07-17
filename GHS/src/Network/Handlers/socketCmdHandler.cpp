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

// ATTENTION: Temperatures will all be passed in C * 100. This is to account
// for precision without passing float values via socket commands. A temperature
// of 10.2 will be passed as 1020 from the client, and stored as such thru
// the program, When being used, it will be divided by 100 to achieve the 
// correct float value.

// ATTENTION: Most commands use bitwise, read comments for format.

namespace Comms {

// Requires command data, buffer, and buffer size. Executes the command passed,
// and compiles response into json and replies. Temperature will be passed as
// an int which is a float * 100 (10.2 is 1020).
void SOCKHAND::compileData(cmdData &data, char* buffer, size_t size) {
    int written{-1}; // Ensures the snprintf is working by checking write size.
    bool writeLog = true; // Set to false by functions not meant to log.

    // reply is the typical reply to a request, with the exception of the
    // GET_ALL request. Used throughout the function.
    char reply[SKT_REPLY_SIZE] = "{\"status\":%d,\"msg\":\"%s\",\"supp\":%d,"
    "\"id\":\"%s\"}"; 

    // All commands work from here. CMD list is on header doc.
    switch(data.cmd) {

        // Gets all sensor and some system data and sends JSON back to client.
        // This is the reason the buffer is large, due to the large amount
        // of data required to pass to the client.
        case CMDS::GET_ALL: {
        
        writeLog = false; // Commonly used command, do not want to log.
        static bool copyBuf = true; // Copies working to master buffer.

        // Commonly used pointers in the scope of GET_ALL. Declared them here
        // to avoid using verbose commands.
        Clock::TIME* dtg = Clock::DateTime::get()->getTime();
        Peripheral::TempHum* th = Peripheral::TempHum::get();
        Peripheral::Timer* re0Timer = SOCKHAND::Relays[0].getTimer();
        Peripheral::Timer* re1Timer = SOCKHAND::Relays[1].getTimer();
        Peripheral::Timer* re2Timer = SOCKHAND::Relays[2].getTimer();
        Peripheral::Timer* re3Timer = SOCKHAND::Relays[3].getTimer();
        Peripheral::Soil* soil = Peripheral::Soil::get();
        Peripheral::Light* light = Peripheral::Light::get();

        // Primary JSON response object. Populates every setting and value that
        // is critical to the operation of this device, and returns it to the
        // client. Ensure client uses same JSON and same commands.
        written = snprintf(buffer, size,  
        "{\"firmv\":\"%s\",\"id\":\"%s\",\"newLog\":%d,\"netMode\":%u,"
        "\"sysTime\":%lu,\"hhmmss\":\"%d:%d:%d\",\"day\":%u,\"timeCalib\":%d,"
        "\"re0\":%d,\"re0TimerEn\":%d,\"re0TimerOn\":%zu,\"re0TimerOff\":%zu,"
        "\"re1\":%d,\"re1TimerEn\":%d,\"re1TimerOn\":%zu,\"re1TimerOff\":%zu,"
        "\"re2\":%d,\"re2TimerEn\":%d,\"re2TimerOn\":%zu,\"re2TimerOff\":%zu,"
        "\"re3\":%d,\"re3TimerEn\":%d,\"re3TimerOn\":%zu,\"re3TimerOff\":%zu,"
        "\"re0Days\":%u,\"re1Days\":%u,\"re2Days\":%u,\"re3Days\":%u,"
        "\"re0Qty\":%u,\"re1Qty\":%u,\"re2Qty\":%u,\"re3Qty\":%u,"
        "\"re0Man\":%u,\"re1Man\":%u,\"re2Man\":%u,\"re3Man\":%u,"
        "\"temp\":%.2f,\"tempRe\":%d,\"tempReCond\":%u,\"tempReVal\":%d,"
        "\"tempAltCond\":%u,\"tempAltVal\":%d,"
        "\"hum\":%.2f,\"humRe\":%d,\"humReCond\":%u,\"humReVal\":%d,"
        "\"humAltCond\":%u,\"humAltVal\":%d,"
        "\"SHTUp\":%d,\"tempAvg\":%0.1f,\"humAvg\":%0.1f,"
        "\"tempAvgPrev\":%0.1f,\"humAvgPrev\":%0.1f,"
        "\"soil0\":%d,\"soil0AltCond\":%u,\"soil0AltVal\":%d,\"soil0Up\":%d,"
        "\"soil1\":%d,\"soil1AltCond\":%u,\"soil1AltVal\":%d,\"soil1Up\":%d,"
        "\"soil2\":%d,\"soil2AltCond\":%u,\"soil2AltVal\":%d,\"soil2Up\":%d,"
        "\"soil3\":%d,\"soil3AltCond\":%u,\"soil3AltVal\":%d,\"soil3Up\":%d,"
        "\"violet\":%u,\"indigo\":%u,\"blue\":%u,\"cyan\":%u,\"green\":%u,"
        "\"yellow\":%u,\"orange\":%u,\"red\":%u,\"nir\":%u,\"clear\":%u,"
        "\"photo\":%d,"
        "\"violetAvg\":%0.1f,\"indigoAvg\":%0.1f,\"blueAvg\":%0.1f,"
        "\"cyanAvg\":%0.1f,\"greenAvg\":%0.1f,\"yellowAvg\":%0.1f,"
        "\"orangeAvg\":%0.1f,\"redAvg\":%0.1f,\"nirAvg\":%0.1f,"
        "\"clearAvg\":%0.1f,\"photoAvg\":%0.1f,"
        "\"violetAvgPrev\":%0.1f,\"indigoAvgPrev\":%0.1f,\"blueAvgPrev\":%0.1f,"
        "\"cyanAvgPrev\":%0.1f,\"greenAvgPrev\":%0.1f,\"yellowAvgPrev\":%0.1f,"
        "\"orangeAvgPrev\":%0.1f,\"redAvgPrev\":%0.1f,\"nirAvgPrev\":%0.1f,"
        "\"clearAvgPrev\":%0.1f,\"photoAvgPrev\":%0.1f,"
        "\"lightRe\":%d,\"lightReCond\":%u,\"lightReVal\":%u,"
        "\"lightDur\":%lu,\"photoUp\":%d,\"specUp\":%d,\"darkVal\":%u,"
        "\"atime\":%u,\"astep\":%u,\"again\":%u,"
        "\"avgClrTime\":%lu}",
        FIRMWARE_VERSION, 
        data.idNum,
        Messaging::MsgLogHandler::get()->newLogAvail(),
        static_cast<uint8_t>(NetMain::getNetType()),
        dtg->raw, dtg->hour, dtg->minute, dtg->second, dtg->day,
        Clock::DateTime::get()->isCalibrated(),
        static_cast<uint8_t>(SOCKHAND::Relays[0].getState()),
        re0Timer->isReady, (size_t)re0Timer->onTime, (size_t)re0Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[1].getState()),
        re1Timer->isReady, (size_t)re1Timer->onTime, (size_t)re1Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[2].getState()),
        re2Timer->isReady, (size_t)re2Timer->onTime, (size_t)re2Timer->offTime,
        static_cast<uint8_t>(SOCKHAND::Relays[3].getState()),
        re3Timer->isReady, (size_t)re3Timer->onTime, (size_t)re3Timer->offTime,
        re0Timer->days, re1Timer->days, re2Timer->days, re3Timer->days,
        SOCKHAND::Relays[0].getQty(), SOCKHAND::Relays[1].getQty(),
        SOCKHAND::Relays[2].getQty(), SOCKHAND::Relays[3].getQty(),
        SOCKHAND::Relays[0].isManual(), SOCKHAND::Relays[1].isManual(),
        SOCKHAND::Relays[2].isManual(), SOCKHAND::Relays[3].isManual(),
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
        th->getFlags()->getFlag(Peripheral::THFLAGS::NO_ERR_DISP),
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
        light->getFlags()->getFlag(Peripheral::LIGHTFLAGS::PHOTO_NO_ERR_DISP),
        light->getFlags()->getFlag(Peripheral::LIGHTFLAGS::SPEC_NO_ERR_DISP),
        light->getConf()->darkVal,
        light->getSpecConf()->ATIME,
        light->getSpecConf()->ASTEP,
        static_cast<uint8_t>(light->getSpecConf()->AGAIN),
        Peripheral::Report::get()->getTime()
        );

        // Copies the working buffer to the master buffer at the end of each
        // hour. This allows the master buffer to be sent to a server for 
        // client preview.
        if (dtg->minute >= 59 && dtg->second >= 50 && copyBuf) {
            memcpy(SOCKHAND::reportBuf, buffer, sizeof(SOCKHAND::reportBuf));
            copyBuf = false;

        } else if (!copyBuf && dtg->minute < 59) {
            copyBuf = true;
        }
        }

        break;

        // Calibrates the system time and day. There are 86400 sec per day, and 
        // the client will use this comamnd and pass the current seconds past
        // midnight and the day number of the week with monday = 0. This will
        // allow all time dependent functions to run as perscribed to real time.
        // Below is the 32-bit bitwise breakdown.
        // 0000 0000   0000 DDDT   TTTT TTTT   TTTT TTTT
        // D = day of the week, Monday is day 0, Sunday is day 6.
        // T = time in seconds, 17-bits max 131071, day is 86400.
        case CMDS::CALIBRATE_TIME: {
        uint8_t day = (data.suppData >> 17) & 0b111; // 3 bits,
        uint32_t time = data.suppData & 0x1FFFF; // 17 bits

        bool dayRange = SOCKHAND::inRange(0, 6, day);
        bool timeRange = SOCKHAND::inRange(0, 86399, time);

        if (!dayRange || !timeRange) {
            written = snprintf(buffer, size, reply, 0, "Time/date out of range", 
                0, data.idNum);
            break; // Break and block if range error.
        }

        Clock::DateTime::get()->calibrate(time, day);
        written = snprintf(buffer, size, reply, 1, 
            "Day/Time calibrated", time, data.idNum);
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

        // Controls relays 1 - 4 by changing the relay status using bitwise
        // operations. Below is the 8-bit bitwise breakdown.
        // RRRR CCCC
        // R = relay number: 0 - 3, 0 is relay 1, 3 is relay 4.
        // C = command: 0 = off, 1 = on, 2 = force off, 3 = force removed
        case CMDS::RELAY_CTRL: {
        uint8_t reNum = (data.suppData >> 4) & 0b1111;
        uint8_t cmd = data.suppData & 0b1111;
        writeLog = false; // Log written in relay class.

        // Run some range checks
        bool reRange = SOCKHAND::inRange(0, 3, reNum);
        bool cmdRange = SOCKHAND::inRange(0, 3, cmd);

        if (!reRange || !cmdRange) {
            written = snprintf(buffer, size, reply, 0, "Re Att Range bust", 
                0, data.idNum);
            break; // Break and block if range error.
        }

        // Register all 4 relays upon first call. Put into array for mapping.
        static uint8_t IDR0 = SOCKHAND::Relays[0].getID(RELAY_MAN_0_TAG);
        static uint8_t IDR1 = SOCKHAND::Relays[1].getID(RELAY_MAN_1_TAG);
        static uint8_t IDR2 = SOCKHAND::Relays[2].getID(RELAY_MAN_2_TAG);
        static uint8_t IDR3 = SOCKHAND::Relays[3].getID(RELAY_MAN_3_TAG);
        static uint8_t IDS[] = {IDR0, IDR1, IDR2, IDR3};

        // Use the renum to make the correct call.
        switch (cmd) {
            case 0: // off
            SOCKHAND::Relays[reNum].off(IDS[reNum]);
            break;

            case 1: // on
            SOCKHAND::Relays[reNum].on(IDS[reNum]);
            break;

            case 2: // force off (shuts relay off regardless of dependency)
            SOCKHAND::Relays[reNum].forceOff(); // ID not required.
            break;

            case 3: // remove the force.
            SOCKHAND::Relays[reNum].removeForce(); // ID not required.
            break;
        }

        written = snprintf(buffer, size, reply, 1, "Relay Change", 
            reNum, data.idNum);
        }

        break;

        // Sets the relay timer on and off times using bitwise operations. 
        // Below is the 32-bit bitwise breakdown. It is a little unusal, but
        // required to keep it within the constraints of an int.
        // RRRR SSSS   SSSS SSSS   SSSS SDDD   DDDD DDDD
        // R = relay number. 0 is 1, 3 is 4.
        // S = start time in seconds. 17-bits, allow max of 131072.
        // D = duration in minutes. 11-bits, allow max of 2048.
        case CMDS::RELAY_TIMER: {
        uint8_t reNum = (data.suppData >> 28) & 0b1111; // 4 bits
        uint32_t start = (data.suppData >> 11) & 0x1FFFF; // 17 bits
        uint16_t dur = data.suppData & 0x7FF; // 11 bits

        // Run range checks.
        bool reRange = SOCKHAND::inRange(0, 3, reNum);
        bool startRange = SOCKHAND::inRange(0, 86399, start, RELAY_TIMER_OFF);

        // If the start condition is to shut the relay off, durRange is set to
        // true since it has no function meaning duration can be anything.
        bool durRange = (start == RELAY_TIMER_OFF) ? true : 
            SOCKHAND::inRange(1, 1439, dur);

        if (!reRange || !startRange || !durRange) {
            written = snprintf(buffer, size, reply, 0, "Re Tmr Range bust", 
                0, data.idNum);

            break; // Break and block if range error.
        }

        // Compute the off seconds based on start time and duration, and set.
        uint32_t finish = (start + (dur * 60)) % 86400;

        if (!SOCKHAND::Relays[reNum].timerSet(start, finish)) {
            written = snprintf(buffer, size, reply, 0, "Re Tmr not set", 
                0, data.idNum);
        } else {
            written = snprintf(buffer, size, reply, 1, "Re Tmr set", reNum, 
                data.idNum);
        }
        }

        break;

        // Sets the days of the week that are applicable to the relay timer.
        // Below is the 12-bit bitwise breakdown. 
        // 0000 RRRR 0SDD DDDM
        // R = relay number. Below bitwise ops do not occur HERE.
        // D = day of week. Placeholder.
        // S = Sunday, bit 6.
        // M = Monday, bit 0.
        case CMDS::RELAY_TIMER_DAY: {
        uint8_t reNum = (data.suppData >> 8) & 0xF;
        uint8_t days = data.suppData & 0b01111111;

        bool reRange = SOCKHAND::inRange(0, 3, reNum);
        bool dayRange = SOCKHAND::inRange(0, 127, days);

        if (!reRange || !dayRange) {
            written = snprintf(buffer, size, reply, 0, "Re Tmr Day Range bust", 
                0, data.idNum);

            break; // Break and block if range error.
        }

        // Set the days to the relay.
        if (!SOCKHAND::Relays[reNum].timerSetDays(days)) { 
            written = snprintf(buffer, size, reply, 0, "Re Tmr days not set", 
                0, data.idNum);

        } else {
            written = snprintf(buffer, size, reply, 1, "Re Tmr days set", reNum, 
                data.idNum);
        }
        }

        break;

        // Attaches a single relay to a peripheral device using bitwise 
        // operations. Below is the 8-bit bitwise breakdown.
        // DDDD RRRR
        // D = device: 0 for temp, 1 for humidity, and 2 for light.
        // R = relay number: 0 - 4, 0 is relay 1, 3 is relay 4, 4 is detach.
        case CMDS::ATTACH_RELAYS: {
        uint8_t device = (data.suppData >> 4) & 0b1111;
        uint8_t reNum = data.suppData & 0b1111;
        writeLog = false; // Will be written to log using attach function.
        Peripheral::Light* lt = Peripheral::Light::get();
        Peripheral::TempHum* th = Peripheral::TempHum::get();

        // Run some range checks
        bool devRange = SOCKHAND::inRange(0, 2, device);
        bool reRange = SOCKHAND::inRange(0, 4, reNum);

        if (!devRange || !reRange) {
            written = snprintf(buffer, size, reply, 0, "Re Att Range bust", 
                0, data.idNum);

            break; // Break and block if range error
        }

        // Checks good, within range. Continue.
        switch (device) {
            case 0: // Temperature
            SOCKHAND::attachRelayTH(reNum, th->getTempConf(), "Temp");
            written = snprintf(buffer, size, reply, 1, "Temp Relay att", 
                reNum, data.idNum);

            break;

            case 1: // Humidity
            SOCKHAND::attachRelayTH(reNum, th->getHumConf(), "Hum");
            written = snprintf(buffer, size, reply, 1, "Hum Relay att", 
                reNum, data.idNum);

            break;

            case 2: // Light
            SOCKHAND::attachRelayLT(reNum, lt->getConf(), "light");
            written = snprintf(buffer, size, reply, 1, "Light Relay att", 
                reNum, data.idNum);

            break;

            default:;
        }
        }

        break;

        // Sets the temperature and humidity conditions and trip values for
        // relay and alerts using bitwise operations. 
        // Below is the 32-bit int bitwise breakdown.
        // 0000 00SR   0000 00CC   VVVV VVVV   VVVV VVVV
        // S = sensor type: 0 for Humidity, 1 for Temperature
        // R = relay/alt: 0 for relay, 1 for Alert
        // C = condition: 0 = less than, 1 = greater than, 2 = none.
        // V = value: 16 but integer value supports temps to 655 C due to 
        //     value being passed as a 2 point float mult by 100.
        case CMDS::SET_TEMPHUM: {
        uint8_t sensor = (data.suppData >> 25) & 0b1;
        uint8_t controlType = (data.suppData >> 24) & 0b1;
        uint8_t condition = (data.suppData >> 16) & 0b11;
        int16_t value = data.suppData & 0xFFFF; // Allows negative for temp.
        Peripheral::TH_TRIP_CONFIG* conf = nullptr; // Will be set later.
        
        // Alert conditions which will be populated by the condition value,
        // by mapping it to the array index. 
        const Peripheral::ALTCOND ACOND[] = {Peripheral::ALTCOND::LESS_THAN,
            Peripheral::ALTCOND::GTR_THAN, Peripheral::ALTCOND::NONE};

        // Relay conditions, work just like alert condition above.
        const Peripheral::RECOND RECOND[] = {Peripheral::RECOND::LESS_THAN,
            Peripheral::RECOND::GTR_THAN, Peripheral::RECOND::NONE};

        // Run range checks. Sensor val of 1 is temperature.
        bool condRange = SOCKHAND::inRange(0, 2, condition);
        bool valRange = (sensor == 1) ? 
            SOCKHAND::inRange(SHT_MIN, SHT_MAX, value, -999, 100) :
            SOCKHAND::inRange(SHT_MIN_HUM, SHT_MAX_HUM, value);

        // Manipulate the tertiary operator, which will already check the 
        // range. If condition = 2, the val range doesnt matter, it will be
        // set to true.
        if (condition == 2) valRange = true;

        if (!condRange || !valRange) {
            written = snprintf(buffer, size, reply, 0, "TH Range bust", 
                0, data.idNum);

            break; // Break if range error
        }
       
        // Ranges are good, go ahead and assign values.

        // Sets the correct pointer depending on setting of bit 25.
        if (sensor == 1) { // Indicates temperature.
            conf = Peripheral::TempHum::get()->getTempConf();
        } else { // Indicates humidity.
            conf = Peripheral::TempHum::get()->getHumConf();
        }

        // Sets either the relay or alert based on bit 24. If the conditon is
        // NONE or value 2, set tripval to 0.
        if (controlType == 1) { // Indicates Alert

            // If using alert, ensure you are in station mode.
            if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {
                break; // break and block.
            }

            conf->alt.condition = ACOND[condition];
            conf->alt.tripVal = (condition == 2) ? 0 : value;
            written = snprintf(buffer, size, reply, 1, "TEMPHUM alert set", 
                0, data.idNum);

        } else { // Indicates relay.
            conf->relay.condition = RECOND[condition];
            conf->relay.tripVal = (condition == 2) ? 0 : value;
            written = snprintf(buffer, size, reply, 1, "TEMPHUM relay set", 
                0, data.idNum);
        }
        }

        break;

        // Sets the soil alert conditions and trip values using bitwise 
        // operations. Below is the 32-bit bitwise breakdown.
        // 0000 0000   SSSS 00CC   VVVV VVVV   VVVV VVVV
        // S = soil sensor num. 0 is 1, 3 is 4.
        // C = alert condition. 0 = less than, 1 = gtr than, 2 = none.
        // V = 16 bit integer value. Max is 32767 per datasheet.
        case CMDS::SET_SOIL: {
        
        // Ensure you are conncted to the station mode for alert features.
        if (!SOCKHAND::checkSTA(written, buffer, size, reply, data.idNum)) {
            break; // Break and block;
        }

        // If good, extract data.
        uint8_t num = (data.suppData >> 20) & 0b1111;
        uint8_t cond = (data.suppData >> 16) & 0b11;
        uint16_t val = data.suppData & 0xFFFF;

        // Alert conditions which will be populated by the condition value,
        // by mapping it to the array index. 
        const Peripheral::ALTCOND ACOND[] = {Peripheral::ALTCOND::LESS_THAN,
            Peripheral::ALTCOND::GTR_THAN, Peripheral::ALTCOND::NONE};

        // Run some range checks
        bool numRange = SOCKHAND::inRange(0, 3, num);
        bool condRange = SOCKHAND::inRange(0, 2, cond);

        // Checks if condition is set to NONE, If so, the val doesn't matter
        // and range check can be set to true. Otherwise, range check occurs.
        bool valRange = (cond == 2) ? true : 
            SOCKHAND::inRange(SOIL_MIN, SOIL_MAX, val);

        if (!numRange || !condRange || !valRange) {
            written = snprintf(buffer, size, reply, 0, "Soil Range bust", 
                num, data.idNum);

            break; // break and block if range err.
        }

        // Ranges are good, connected to STA, establish sensor configuration.
        Peripheral::AlertConfigSo* conf = 
            Peripheral::Soil::get()->getConfig(num);

        conf->condition = ACOND[cond]; // Set condition using array map.
        conf->tripVal = (cond == 2) ? 0 : val; // If NONE, set to 0.

        written = snprintf(buffer, size, reply, 1, "Soil alt set", 
            num, data.idNum);
        }

        break;

        // Sets the light conditions and trip values for the relay. Light has
        // the dark value which begins the duration of light, and the relay 
        // trip values. Below is the 32-bit bitwise breakdown.
        // 0000 0000   000T 00CC   VVVV VVVV   VVVV VVVV
        // T = type, 0 for dark, 1 for photo.
        // C = condition. 0 = less than, 1 = gtr than, 2 = none.
        // V = value, 16 bits, max 65535.
        case CMDS::SET_LIGHT: {
        uint8_t type = (data.suppData >> 20) & 0b1;
        uint8_t cond = (data.suppData >> 16) & 0b11;
        uint16_t value = data.suppData & 0xFFFF;

        // Relay conditions, work just like alert condition above.
        const Peripheral::RECOND RECOND[] = {Peripheral::RECOND::LESS_THAN,
            Peripheral::RECOND::GTR_THAN, Peripheral::RECOND::NONE};

        bool valRange = SOCKHAND::inRange(PHOTO_MIN, PHOTO_MAX, value);

        // Sets to true if type is dark to bypass checks.
        bool condRange = (type == 0) ? true : SOCKHAND::inRange(0, 2, cond);

        if (!valRange || !condRange) {
            written = snprintf(buffer, size, reply, 0, "Light Range bust", 
                0, data.idNum);

            break; // Break if range error
        }

        // Checks are good, go ahead an make the setting change.
        Peripheral::RelayConfigLight* conf = 
            Peripheral::Light::get()->getConf();

        if (type == 0) { // Indicates dark value

            conf->darkVal = value;
            written = snprintf(buffer, size, reply, 1, "Dark Val", 
                value, data.idNum);

        } else { // Indicates photo value. Type = 1.

            conf->condition = RECOND[cond];
            conf->tripVal = (cond == 2) ? 0 : value;
            written = snprintf(buffer, size, reply, 1, "Photo Relay Set", 
                conf->tripVal, data.idNum);
        }
        }

        break;

        // Sets the spectral integration time which is composed of both the 
        // ATIME and ASTEP. Below is the 32 bit bitwise breakdown.
        // 0000 0000   SSSS SSSS   SSSS SSSS   TTTT TTTT
        // S = ASTEP, 16 bit integer.
        // T = ATIME, 8 bit integer.
        case CMDS::SET_SPEC_INTEGRATION_TIME: {
        uint8_t ATIME = data.suppData & 0xFF; // Extract the ATIME.
        uint16_t ASTEP = (data.suppData  >> 8) & 0xFFFF; // Extract the ASTEP.

        if (!SOCKHAND::inRange(0, 255, ATIME) || 
            !SOCKHAND::inRange(0, 65534, ASTEP)) {

            written = snprintf(buffer, size, reply, 0, "Lgt Integ RangeErr", 0, 
                data.idNum);

        } else {

            Peripheral::Light* lt = Peripheral::Light::get();

            if (lt->setASTEP(ASTEP)) {
                // vTaskDelay(pdMS_TO_TICKS(20)); // Uncomment if future err

                if (lt->setATIME(ATIME)) {
                    written = snprintf(buffer, size, reply, 1, "Lgt Integ Set", 
                        0, data.idNum);
                } else {
                    written = snprintf(buffer, size, reply, 0, "Lgt Integ Err", 
                        0, data.idNum);
                }

            } else {
                written = snprintf(buffer, size, reply, 1, "Lgt Integ Err", 0, 
                    data.idNum);
            }
        }
        }
            
        break;

        // Sets the spectral gain. This is typically done dynamically. The
        // values are 0 - 10, and correspond to the AS7341 driver AGAIN enum.
        case CMDS::SET_SPEC_GAIN: {
            if (!SOCKHAND::inRange(0, 10, data.suppData)) {
                written = snprintf(buffer, size, reply, 0, "Lgt Gain RangeErr", 
                    data.suppData, data.idNum);

            } else {
                bool set = Peripheral::Light::get()->setAGAIN(
                    static_cast<AS7341_DRVR::AGAIN>(data.suppData));

                if (set) {
                    written = snprintf(buffer, size, reply, 1, "Lgt Gain Set", 
                        data.suppData, data.idNum);

                } else {
                    written = snprintf(buffer, size, reply, 0, "Lgt Gain Err", 
                        data.suppData, data.idNum);
                }
            }
        }

        break;

        // Clears the averages for the temphum, and the light. Soil averages are
        // not captured considering their relatively slow change. Below is
        // the 8-bit breakdown, there are no bitwise operations.
        // 0000 TTTT
        // T = type of average to clear. 0 is temp/hum, 1 is light, 2 is both.
        case CMDS::CLEAR_AVERAGES: {
        uint8_t type = data.suppData;
        bool clearAll = (type == 2);

        // Check range
        bool typeRange = SOCKHAND::inRange(0, 2, type);

        if (!typeRange) {
            written = snprintf(buffer, size, reply, 0, "Clr Avg Range bust", 
                0, data.idNum);

            break; // Break if range error
        }

        // Clear averages. If type is passed as two, manipulate data to delete
        // all.
        if (clearAll) type = 0;
        if (type == 0) Peripheral::TempHum::get()->clearAverages();
        if (clearAll) type = 1;
        if (type == 1) Peripheral::Light::get()->clearAverages();
        
        written = snprintf(buffer, size, reply, 1, "Avg Clear", 0, 
            data.idNum);
        }

        break;

        // Sets the second past midnight that the averages will be cleared. 
        // Defaults to 23:59:00 or 86340.
        case CMDS::CLEAR_AVG_SET_TIME:

        if (!SOCKHAND::inRange(0, MAX_SET_TIME, data.suppData)) {

            written = snprintf(buffer, size, reply, 0, "Avg Clr timer bust", 0, 
                data.idNum);
        } else {
            Peripheral::Report::get()->setTimer(data.suppData);

            written = snprintf(buffer, size, reply, 1, "Avg Clr Time set", 
                data.suppData, data.idNum);
        }

        break;

        // When called, the device will save all configuration settings to the
        // NVS and restart the system.
        case CMDS::SAVE_AND_RESTART:
        NVS::settingSaver::get()->saveAndRestart();
        break;

        default:
        break;

        // When called, device will reply in json format, all trends of temp,
        // humidity, and spectral light, based on hourly readings. JSON return
        // uses temp, hum, and each color captured, nir, and clear.
        case CMDS::GET_TRENDS: 
        if (!SOCKHAND::inRange(1, TREND_HOURS, data.suppData)) {
            written = snprintf(buffer, size, reply, 0, "Trend hour rangeErr", 0, 
                data.idNum);

        } else { 

            writeLog = false; // Prevent large log of data
            Peripheral::TH_Trends* th = Peripheral::TempHum::get()->getTrends();

            Peripheral::Light_Trends* lt = 
                Peripheral::Light::get()->getTrends();

            int iter = data.suppData; // Iterations or hours to compute.
            
           // Create all float buffers to populate data into.
            char tempBuf[SKT_REPLY_SIZE]{0};
            char humBuf[SKT_REPLY_SIZE]{0};

            // Populate buffers above with requested data.
            SOCKHAND::THtrend(th->temp, tempBuf, SKT_REPLY_SIZE, iter);
            SOCKHAND::THtrend(th->hum, humBuf, SKT_REPLY_SIZE, iter);

            // Create all unit16_t buffers to populate data into.
            char ltClr[SKT_REPLY_SIZE]{0}; char ltVio[SKT_REPLY_SIZE]{0}; 
            char ltInd[SKT_REPLY_SIZE]{0}; char ltBlu[SKT_REPLY_SIZE]{0}; 
            char ltCya[SKT_REPLY_SIZE]{0}; char ltGre[SKT_REPLY_SIZE]{0}; 
            char ltYel[SKT_REPLY_SIZE]{0}; char ltOra[SKT_REPLY_SIZE]{0}; 
            char ltRed[SKT_REPLY_SIZE]{0}; 
            char ltNir[SKT_REPLY_SIZE]{0}; // Near infrared
            char ltPho[SKT_REPLY_SIZE]{0}; // Photoresistor

            // Populate buffers above with requested data.
            SOCKHAND::lightTrend(lt->clear, ltClr, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->violet, ltVio, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->indigo, ltInd, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->blue, ltBlu, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->cyan, ltCya, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->green, ltGre, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->yellow, ltYel, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->orange, ltOra, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->red, ltRed, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->nir, ltNir, SKT_REPLY_SIZE, iter);
            SOCKHAND::lightTrend(lt->photo, ltPho, SKT_REPLY_SIZE, iter);

            // Write JSON data back to client.
            written = snprintf(buffer, size, 
            "{\"id\":\"%s\",\"temp\":[%s],\"hum\":[%s],"
            "\"clear\":[%s],\"violet\":[%s],\"indigo\":[%s],\"blue\":[%s],"
            "\"cyan\":[%s],\"green\":[%s],\"yellow\":[%s],\"orange\":[%s],"
            "\"red\":[%s],\"nir\":[%s],\"photo\":[%s]}",
            data.idNum, tempBuf, humBuf, ltClr, ltVio, ltInd, ltBlu, ltCya,
            ltGre, ltYel, ltOra, ltRed, ltNir, ltPho);
        }

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

// Requires the lower and upper bounds, the value, the exception value, and the
// multiplier. The exception value is if a value is out of range, but equals 
// the exception, it will return true. Such as a relay time in second setting 
// can only be between 0 and 86400, but 99999 is the signal to shut it off. The
// default is set to -999. The multiplier is default to 1. This is used for 
// temp, since float values are required, the value will be multiplied by
// 100, such as 10.2C will be passed as 1020, so the normal range of -39 to 
// 124 will be multiplied by 100 to account for that precision. Returns true
// if within range, and false if not.
bool SOCKHAND::inRange(int lower, int upper, int value, int exception, 
    int multiplier) {

        return ((value >= (lower * multiplier)) && 
               (value <= (upper * multiplier))) ||
               (value == exception);
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

// Requires float pointer to array, char buffer, the buffersize, and number
// of iterations, typically 1 to 12. Populates a json array, with the requested
// values to be replied back to the socket client.
void SOCKHAND::THtrend(float *arr, char* buf, size_t bufSize, int iter) {

    if (arr == nullptr || buf == nullptr) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s nullptr passed", SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        return; // Block
    }

    char miniBuf[32]{0}; // Used as a temporary buffer for creation.

    for (int i = 0; i < iter; i++) {
        if (i == (iter - 1)) { // Changes last index to remove comma.
            snprintf(miniBuf, sizeof(miniBuf), "%0.1f", arr[i]);
        } else {
            snprintf(miniBuf, sizeof(miniBuf), "%0.1f, ", arr[i]);
        }

        size_t remaining = bufSize - strlen(buf) - 1; // Null term.
        strncat(buf, miniBuf, remaining); // Add new entry.
    }
}

// Requires uint16_t pointer to array, char buffer, the buffersize, and number
// of iterations, typically 1 to 12. Populates a json array, with the requested
// values to be replied back to the socket client.
void SOCKHAND::lightTrend(uint16_t* arr, char* buf, size_t bufSize, 
    int iter) {

    if (arr == nullptr || buf == nullptr) {
        snprintf(MASTERHAND::log, sizeof(MASTERHAND::log), 
            "%s nullptr passed", SOCKHAND::tag);

        MASTERHAND::sendErr(MASTERHAND::log);
        return; // Block
    }

    char miniBuf[32]{0}; // Used as a temp buffer for creation.

    for (int i = 0; i < iter; i++) {
        if (i == (iter - 1)) { // Changes last index to remove comma
            snprintf(miniBuf, sizeof(miniBuf), "%u", arr[i]);
        } else {
            snprintf(miniBuf, sizeof(miniBuf), "%u, ", arr[i]);
        }

        size_t remaining = bufSize - strlen(buf) - 1; // null term
        strncat(buf, miniBuf, remaining);
    }
}

// Requires relay number and pointer to TempHum configuration. Ensures relay
// number is between 0 and 4, 0 - 3 (relays 1 - 4), and 4 sets the relay to
// nullptr and remove functionality. 
void SOCKHAND::attachRelayTH(uint8_t relayNum, 
    Peripheral::TH_TRIP_CONFIG* conf, const char* caller) {
        
    if (relayNum < 4) { // Checks for valid relay number.
        // If active relay is currently assigned, ensures that it is 
        // removed from control and shut off prior to a reissue.
        if (conf->relay.relay != nullptr) {
            conf->relay.relay->removeID(conf->relay.controlID);
        }

        conf->relay.relay = &SOCKHAND::Relays[relayNum];
        conf->relay.controlID = SOCKHAND::Relays[relayNum].getID(caller);
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

// Same as temphum relay.
void SOCKHAND::attachRelayLT(uint8_t relayNum, 
    Peripheral::RelayConfigLight* conf, const char* caller) {

    if (relayNum < 4) { // Checks for valid relay number.
        // If active relay is currently assigned, ensures that it is 
        // removed from control and shut off prior to a reissue.
        if (conf->relay != nullptr) {
            conf->relay->removeID(conf->controlID);
        }

        conf->relay = &SOCKHAND::Relays[relayNum];
        conf->controlID = SOCKHAND::Relays[relayNum].getID(caller);
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