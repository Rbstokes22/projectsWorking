const {manageSocket, updateDev, sktSend, poll, startPoll, stopPoll} = 
    require("../devMgr/newDevice.methods");

const {devMap} = require("./config.devMgr");

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

    this.manageSocket = manageSocket;
    this.updateDev = updateDev;
    this.sktSend = sktSend;
    this.poll = poll;
    this.startPoll = startPoll;
    this.stopPoll = stopPoll;
    
    // Establish socket immediately upon creation
    this.manageSocket();

    devMap[this.mDNS] = this;

    return this;
};

module.exports = {newDevice, devMap};
