const {devMap, getID} = require("../devMgr/newDevice");
const {config} = require("../../config/config");
const {SKT_CMD} = require("../../Common/socketCmds");

let pollInterval = null;

// Currently on polling. We have proper UDP check in and handling to include
// websocket management. When poll is called, the resposne will go to 
// handleDevice. This is where we need to manage everything. handleDevice is
// on the newdevice method, but it might need its own. I am not sure yet. 
// implement functionality that mirros the current web client, so that call
// backs can be used as well, as long as that makes sense.

const poll = () => {

    Object.keys(devMap).forEach(mdns => {

        if (devMap[mdns].isUp) {
            devMap[mdns].send(`${SKT_CMD["GET_ALL"]}/0/${getID()}`);
        }
    });
}

const startPoll = () => {
    try {
        pollInterval = setInterval(poll, config.POLL_FREQ);
    } catch (e) {
        console.error(e);
    }
}

const stopPoll = () => {
    try {
        clearInterval(pollInterval);
    } catch (e) {
        console.error(e);
    }
}

module.exports = {startPoll, stopPoll};