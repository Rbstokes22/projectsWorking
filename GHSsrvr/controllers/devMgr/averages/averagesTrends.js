const {devMap} = require("../config.devMgr");
const {getTime} = require("../../../Common/timeMgr");
let avgClrFlag = false; // Used to track average clearing.

// Rounds all averages to decimal precision. Iterates the devices averages,
// and converts them to avoid have several decimal places.
const roundAvgs = function(mdnsKey, decPlaces) {

    const avgs = devMap[mdnsKey].averages;

    Object.keys(avgs).forEach(key => {

        if (typeof avgs[key] === "number") { // Check if number.
            avgs[key] = Number(avgs[key].toFixed(decPlaces));

        } else if (typeof avgs[key] === "object") { // Check if object.

            if (Array.isArray(avgs[key])) {
                avgs[key].forEach((v, idx) => {
                    avgs[key][idx] = Number(avgs[key][idx].toFixed(decPlaces));
                });

            } else {
                Object.keys(avgs[key]).forEach(obKey => {
                    avgs[key][obKey] = 
                        Number(avgs[key][obKey].toFixed(decPlaces));
                });
            }
        }
    });
}

// Requires the mdnsKey value that maps to devMap. Iterates all of the data
// from the esp, extracting prescribed kv pairs, and computes the running avgs.
// Averages will be running in hour long intervals before resetting, capturing
// data to trends.
// WARNING. Ensure the keys used here correspond with the esp.
const handleAverages = function(mdnsKey) {
    
    const dev = devMap[mdnsKey];
    const sysData = dev.devSysData;

    // Note the +1 to the poll count ensures never dividing by 0.

    // Set the temp and humidity averages.
    if (sysData["SHTRdOK"]) { // Temp/Hum read is good.

        const deltaT = sysData["temp"] - dev.averages["temp"];
        const deltaH = sysData["hum"] - dev.averages["hum"];
        dev.averages["temphumP"] += 1; // Increment denominator
        dev.averages["temp"] += (deltaT / dev.averages["temphumP"]);
        dev.averages["hum"] += (deltaH / dev.averages["temphumP"]);
    }

    // Iterate each of the soil sensors, setting the soil averages.
    for (let i = 0; i < 4; i++) {

        if (!sysData[`soil${i}RdOK`]) continue;

        // Good data
        const delta = sysData[`soil${i}`] - dev.averages["soil"][i];
        dev.averages["soilP"][i] += 1; // increment denominator.
        dev.averages["soil"][i] += (delta / dev.averages["soilP"][i]);
    }

    // Set the photo resistor averages.
    if (sysData["photoRdOK"]) {

        const delta = sysData["photo"] - dev.averages["photo"];
        dev.averages["photoP"] += 1; // increment denominator.
        dev.averages["photo"] += (delta / dev.averages["photoP"]);
    }
    
    // The the spectral data averages.
    if (sysData["specRdOK"]) {

        // All object keys for each channel of the spec read. 
        const channels = ["violet", "indigo", "blue", "cyan", "green", 
            "yellow", "orange", "red", "nir", "clear"
        ];

        dev.averages["specP"] += 1; // increment denominator once for all chan.

        channels.forEach(channel => {

            const delta = sysData[channel] - dev.averages["spec"][channel];
            dev.averages["spec"][channel] += (delta / dev.averages["specP"]);
        });
    }

    roundAvgs(mdnsKey, 2); // Rounds averages to ensure two point precision.
}

// Requires the hour. When called, the device manager will clone the averages
// from the current hour, into the trends for that hour. Upon doing so, it will
// reset all averages to 0. If device is down, averages will never compute, 
// which will stores 0's. 
const clearAllAvgs = function(hour) {

    Object.keys(devMap).forEach(device => {

        devMap[device]["trends"][hour] = 
            structuredClone(devMap[device]["averages"]);

        devMap[device].clearAvgs();
    });
}

// Requires no params. Runs logic to determine when the next 59th minute past
// the hour occurs, and sets timeout to clear all averages, as well as runs a
// reschedule for the next event.
const avgClearScheduler = function() { 

    const now = new getTime(); // Current time
    const BD = now.getHourBreakdown(); // Breakdown parts by hour min/sec/ms
    let delayS = 0; // delay in seconds, will be conv to ms upon application.
    const _59thMin = 3540; // 59th minute in seconds.

    // See when to run the first timeout by analyizing the current time.
    // If time is in the 59th minute, the probabilistic case after the first 
    // run, compute the remaining seconds in the hour, and add an hour to it
    // to capture the next run. 
    if (now.min >= 59) {
        const remaining = 3600 - BD.sec;
        delayS = remaining + _59thMin;

    // This is the probabilistic case in the first run. Compute how many sec
    // remain between now and the 59th minute. Schedule for that.
    } else {
        delayS = _59thMin - BD.sec; 
    }

    // Runs to clear averages and reschedule next clear.
    setTimeout(() => {
        const _now = new getTime();
        clearAllAvgs(_now.hour);
        avgClearScheduler();

    }, (delayS * 1000));
}

avgClearScheduler(); // Run upon init.

module.exports = {handleAverages};