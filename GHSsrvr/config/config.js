
const config = {

    // mDNS
    mDNSConfig:  { // MDNS will be broadcast with this info. 
        name: "ghmain",
        type: "http",
        port: 5702,
        host: "ghmain.local",
        txt: {version: "1.0"}
    },

    // UDP for heartbeat
    UDP_PORT: 6842,
    DEV_MAP_EXPIRE: 5000, // Clears devices that have not checked in in n ms.
    DEV_MAP_CHK_INT: 1000, // Interval to check for expired devices.


    // Webpath to PWA site
    WEB_PATH: "https://www.mysterygraphlabs.com",

    // RAMDISK path will be init to undefined, and set upon calling 
    // resoloveRAMPath().
    RAM_PATH: undefined,

    // Polling data 
    POLL_FREQ: 1000, // polls each device at this interval accumulating data.
    POLL_SKT_PORT: 6153, // Port to reply to with polling data from esp.
    RE_INIT_SKT_DELTA: 5000, // If no comm in n ms, restart web socket to esp.



    
}

module.exports = {config};
