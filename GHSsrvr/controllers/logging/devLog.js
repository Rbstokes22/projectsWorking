const {devMap} = require("../devMgr/config.devMgr");
const {config} = require("../../config/config");
const dgram = require("dgram");
const socket = dgram.createSocket("udp4"); // UDP socket to comm with esp.

socket.on("message", (msg, rInfo) => {

    try {

        let log = JSON.parse(msg.toString());
        const mDNS = log.mdns;
        const entry = log.entry;
        console.log(entry);
        
        devMap[mDNS].log["lastSysLog"] = 0; // USE REGEX TO GET SYS LOG TIME

        // Run logic here to get the new log entry, if this entry is < last
        // log entry, this means dev restarted, If not, add as normal. Design
        // the log to always ensure it is greater than last before appending,
        // if not, or if it is a first log, get entire log from device, find
        // where the newest log entries fit in, and add them there. The current
        // array might work, but might not be good enough, easily workable though.





    } catch (err) {
        console.error(err);
    }




});

socket.bind(config.UDP_PORT_LOG, () => {
    console.log(`Log UDP listener up on UDP port ${config.UDP_PORT_LOG}`);
});

