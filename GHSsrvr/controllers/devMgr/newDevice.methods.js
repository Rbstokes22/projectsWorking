const WebSocket = require('ws');

const handleDevice = (dev, msg) => {
    console.log(msg);
}

// Manages web socket connection between this server acting as a client, to
// the esp32 device socket server.
const manageSocket = function() {
    // If already connected and open, do nothing
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        return;
    }

    // If socket exists but is not open, force terminate. This prevents
    // termination if websocket is CLOSE or CLOSING.
    if (this.ws && this.ws.readyState !== WebSocket.CLOSING) {
        try { this.ws.terminate(); } catch (e) {}
    }

    // Create new websocket
    this.ws = new WebSocket(this.sockURL);

    this.ws.on("open", () => {
        console.log(`WS connection @ ${this.ip} OPEN`);
    });

    this.ws.on("message", (msg) => {
        handleDevice(this, msg); 
    });

    this.ws.on("close", () => {
        console.log(`WS connection @ ${this.ip} CLOSED`);
        this.ws = null;
        this.isUp = false;
    });

    this.ws.on("error", () => {
        // This can pollute log entries, commented out intentionally.
        // console.error(`WS ERROR @ ${this.ip}`);
    });
}

// Updates each device upon each heartbeat to ensure proper management 
// throughout the program. Calls manageSocket() to ensure that websockets are
// established and destroyed as needed, if devices go offline or what not.
const updateDev = function(devData, ip) {
    this.devData = devData;
    this.ip = ip;
    this.lastSeen = Date.now();
    this.isUp = true;

    const _name = re.exec(devData.mdns);
    this.name = _name ? _name[1] : devData.mdns;

    this.sockURL = `ws://${this.ip}/ws`
    this.manageSocket();
}

// Requires socket message. Ensures socket is open before sending. Returns true
// if successful, and false if not.
const sktSend = function(msg) {

    if (!this.ws) {
        console.warn(`No webskt for ${this.name} @ ${this.ip}`);
        return false;
    }

    if (this.ws.readyState !== WebSocket.OPEN) {
        console.warn(`Webskt not open for ${this.name} @ ${this.ip}`);
        return false;
    }

    try {
        this.ws.send(msg);
        return true;
        
    } catch (err) {
        console.error(`Failed to send skt msg to ${this.name}:`, err);
        return false;
    }
}

module.exports = {manageSocket, updateDev, sktSend};