// ATTENTION. Ensure that the CMDS array matches the array on the esp32 enum
// on socketHandler.hpp.

const SKT_CMD = {}
const CMDS = [
    "GET_ALL", "CALIBRATE_TIME", "NEW_LOG_RCVD", 
    "RELAY_CTRL", "RELAY_TIMER", "RELAY_TIMER_DAY", "ATTACH_RELAYS", 
    "SET_TEMPHUM", "SET_SOIL", "SET_LIGHT", "SET_SPEC_INTEGRATION_TIME", 
    "SET_SPEC_GAIN", "CLEAR_AVERAGES", "CLEAR_AVG_SET_TIME", 
    "SAVE_AND_RESTART", "GET_TRENDS"
];

// Iterate each CMD, populate SKT_CMD and add 1 to the index value to match
// he enumeration in the esp32.
CMDS.forEach((CMD, idx) => {
    SKT_CMD[CMD] = idx+1;
});

module.exports = {SKT_CMD};