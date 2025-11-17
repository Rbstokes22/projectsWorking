const dgram = require("dgram");
const UDP_PORT = 6842;
const DEV_MAP_CLEAR = 5000; // Clear inactive after n ms.
const DEV_MAP_INTERVAL = 1000; // run interval at n frequency.

const socket = dgram.createSocket("udp4"); // UDP socket to comm with esp32.

const devMap = {}; // Holds the data for each device.

// Set even for socket upon message receipt. Add device info to the device 
// map indicating it is ready for use.
socket.on("message", (msg, rInfo) => {

    try { // Parse and populate JSON to the device map.
        const hb = JSON.parse(msg.toString());
        hb.lastSeen = Date.now();
        hb.ip = rInfo.address;

        devMap[hb.mdns] = hb;

    } catch (e) {}
});

// Bind the socket to the udp port.
socket.bind(UDP_PORT, () => {
    console.log(`Heartbeat UDP listener up on UDP port ${UDP_PORT}`);
});

// Set interval to run at n seconds. This iterates the device map looking for
// expired devices, or those that havent checked in by n seconds. If expired,
// the will be cleared from the active device map.
setInterval(() => {

    const current = Date.now(); 

    Object.keys(devMap).forEach(mdns => {
        if ((current - devMap[mdns]) > DEV_MAP_CLEAR) {
            devMap.delete[mdns];
        }
    });

}, DEV_MAP_INTERVAL);

module.exports = { devMap };