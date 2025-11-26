const {manageSocket, updateDev, sktSend, poll, startPoll, stopPoll, 
    clearAvgs} = require("../newDev/newDevice.methods");

const {devMap} = require("../config.devMgr");

// Adds a new Device to the devMap. Will carry all information related to
// the device to include websocket management, updates, and the polled data
// received from the device once sockets are established.
const newDevice = function (devData, ip) {

    // Regex for each mdns to extract the name. 
    const re = /^http:\/\/(greenhouse\d*)\.local$/; 

    this.devData = devData;
    this.ip = ip;
    this.lastSeen = Date.now();
    this.mDNS = devData.mdns;

    const _name = re.exec(this.mDNS);
    this.name = _name ? _name[1] : this.mDNS;

    this.sockURL = `ws://${this.ip}/ws`;
    this.ws = null;
    this.isUp = true;
    this.pollInt = null; // Polling interval
    this.devSysData = {}; // This will store the most current allData string
                          // from the esp device as JS object once parsed.

    // All averages for each sensor. Soil uses an array, since its data is
    // equal and there are 4 sensors. WARNING, ensure that the keys match
    // the keys from the esp allData JSON. This will be used in parsing to
    // compute the averages. Suffixed 'P' = poll ct.
    this.averages = {
        "temp": 0, "hum": 0, "soil": [0, 0, 0, 0], "photo": 0, "spec": {
            "violet": 0, "indigo": 0, "blue": 0, "cyan": 0, "green": 0,
            "yellow": 0, "orange": 0, "red": 0, "nir": 0, "clear": 0
        }, "temphumP": 0, "soilP": [0, 0, 0, 0], "photoP": 0, "specP": 0
    };

    // Stores trends based on the hour. At the top of each hour, averages will
    // be added into object with the key = hour format. Averages then will be
    // cleared, meaning that all trends and averages will use this object.
    this.trends = {}; 

    for (let i = 0; i < 24; i++) { // Formats this.trends for 24 hours.
        this.trends[i] = {};
    }

    this.manageSocket = manageSocket; 
    this.updateDev = updateDev;
    this.sktSend = sktSend;
    this.poll = poll;
    this.startPoll = startPoll;
    this.stopPoll = stopPoll;
    this.clearAvgs = clearAvgs;
    
    // Establish socket immediately upon creation
    this.manageSocket();

    devMap[this.mDNS] = this;

    return this;
};

module.exports = {newDevice, devMap};
