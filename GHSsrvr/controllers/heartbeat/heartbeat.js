const dgram = require("dgram");
const {config} = require("../../config/config");
const socket = dgram.createSocket("udp4"); // UDP socket to comm with esp32.
const {newDevice} = require("../devMgr/newDev/newDevice");
const {devMap} = require("../devMgr/config.devMgr");

// Set even for socket upon message receipt. Add device info to the device 
// map indicating it is ready for use.
socket.on("message", (msg, rInfo) => {
 
    try {

        let devData = null;

        // Parse the JSON UDP message and get remote info
        try {
            devData = JSON.parse(msg.toString());
        } catch (e) {
            console.error("Invalid UDP JSON from device:", err);
            return;
        }

        // JSON is solid proceed. First get mDNS since that is the key used in
        // devMap.
        const mDNS = devData.mdns;

        if (!mDNS) {
            console.error("mDNS not passed from UDP");
            return; // Block if error, mDNS is required to proceed.
        }

        // Next scan to see if this object exists in the devMap.
        const exists = Object.keys(devMap).includes(mDNS);

        // Create a new device if it is not registered in the devMap. It will
        // add itself into the devMap for follow on operations.
        if (!exists) {
            new newDevice(devData, rInfo.address);
            return; // Created device block the rest of the code.
        }

        // Device exists, update and manage appropriately.
        devMap[mDNS].updateDev(devData, rInfo.address);

    } catch (e) {}
});

// Bind the socket to the udp port.
socket.bind(config.UDP_PORT, () => {
    console.log(`Heartbeat UDP listener up on UDP port ${config.UDP_PORT}`);
});

// Set interval to run at n seconds. This iterates the device map looking for
// expired devices, or those that havent checked in by n seconds. If expired,
// they device will have its isUp flag changed to false. This will ensure all
// items are still being tracked through the system and allow better control.
setInterval(() => {

    const current = Date.now(); 

    Object.keys(devMap).forEach(mdns => {

        // Change flags in devMap to false indicating not connected. Add 
        // device to devExp with the time expired.
        if ((current - devMap[mdns].lastSeen) > config.DEV_MAP_EXPIRE && 
            devMap[mdns].isUp == true) {

            devMap[mdns].isUp = false;
            devMap[mdns].timeExp = Date.now(); 
        }
    });

}, config.DEV_MAP_CHK_INT);

