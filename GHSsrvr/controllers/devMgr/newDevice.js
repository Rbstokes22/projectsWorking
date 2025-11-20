const {manageSocket, updateDev, sktSend} = 
    require("../devMgr/newDevice.methods");

const devMap = {}; // Maps and manages all devices.
let sktMsgID = 0; // Will be sent with each socket command to allow async 
                  // returns to know which callback to call. Must be between
                  // 0 and 255. Handled by getID().

// Adds a new Device to the devMap. Will carry all information related to
// the device to include websocket management, updates, and the polled data
// received from the device once sockets are established.
const newDevice = function (devData, ip) {

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
    this.devSysData = {}; // This will store the most current allData string
                          // from the esp device as JS object once parsed.

    this.manageSocket = manageSocket;
    this.updateDev = updateDev;
    this.sktSend = sktSend;
    
    // Establish socket immediately upon creation
    this.manageSocket();

    devMap[this.mDNS] = this;

    return this;
};

// Requires no params. Returns consecutive IDs between 0 and 255 as req by
// esp32 device.
const getID = () => {
    const id = sktMsgID;
    sktMsgID++;
    sktMsgID %= 256; // Keeps value between 0 and 255.
    return id;
}

module.exports = {newDevice, devMap, getID};
