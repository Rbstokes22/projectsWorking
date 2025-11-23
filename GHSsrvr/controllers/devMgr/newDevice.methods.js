const WebSocket = require('ws');
const {handleResponse, getID} = require("./sktHand");
const {getAll} = require("./sktHand.callbacks");
const {config} = require("../../config/config");
const {SKT_CMD} = require("../../Common/socketCmds");

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
        this.stopPoll(); // Stop poll if in a hung state.
        this.isUp = false;

        try { 
            this.ws.terminate(); 
            this.ws = null; // Set back to null for control purposes after term.
        } catch (e) {}
    }

    // Create new websocket
    this.ws = new WebSocket(this.sockURL);

    // Set all event listeners for the web socket upon sucessful creation of 
    // web socket.
    this.ws.on("open", () => {
        console.log(`WS connection @ ${this.ip} OPEN`);
        this.startPoll(); // Start polling once socket is open.
    });

    this.ws.on("message", (msg) => {
        handleResponse(this.mDNS, msg); // Pass devMap key, and message.
    });

    this.ws.on("close", () => {
        console.log(`WS connection @ ${this.ip} CLOSED`);
        this.ws = null;
        this.isUp = false;
        this.stopPoll(); // Stops polling upon socket close.
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

// Requires no params. Polls data from esp device and updates class instance
// devSysData{} to match that of the esp. 
const poll = function() {

    const params = this.mDNS; // Key for devMap.
    const {id, promise} = getID(getAll, params, null);

    promise.catch(err => console.error(err));

    const msg = `${SKT_CMD["GET_ALL"]}/0/${id}`;
    this.sktSend(msg);
}

// Requires no params. Starts polling esp at set interval of 1 Hz freq.
const startPoll = function() {

    try {
        if (this.pollInt !== null) {
            throw("Poll Interval already established");
        }

        // Ensure to wrap this.poll() in the arrow function to bind that 
        // instance to the interval while it is defined, allowing "this".
        this.pollInt = setInterval(() => this.poll(), config.POLL_FREQ);
    } catch (err) {console.error(err);}
}

// Requires no params. Stops poll. Called when socket is shut down.
const stopPoll = function() {
    
    try {
        clearInterval(this.pollInt);
        this.pollInt = null; // Prevents re-assignment in start poll.
    } catch (err) {console.error(err);}
}

module.exports = {manageSocket, updateDev, sktSend, poll, startPoll, stopPoll};