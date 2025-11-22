const sktID_CB = {}; // Stores all callback functions after skt reply.

let sktMsgID = 0; // Will be sent with each socket command to allow async 
                  // returns to know which callback to call. Must be between
                  // 0 and 255. Handled by getID().

// Appends into sktID_CB, by key = ID, the callback functions as well as any
// key value object pairs to update immediately upon response. Returns object
// for use.
const respMap = function(ID, CB, params, KVobj) {
    this.CB = CB;
    this.params = params;
    this.KVobj = KVobj;
    this.timeReq = Date.now();

    sktID_CB[ID] = this;

    return this;
}

// Requires no params, they are defaulted to null. If params are used, CB is
// the callback function, the parameters for the function, and the KV object
// required to update any data values between polling frequency. Returns 
// consecurive IDs between 0 and 255, as required by the esp 32. 
// ATTENTION. KV obj must include the key value pairs for the devices
// devSysData, which is the allData return from the esp. This allows those
// values to be updated, before running the callback handler function. Prevents
// lagging response between polls, and updates data immediately once the esp
// returns a status of 1, meaning the change occured. WARNING. All params
// passed must be able to be used by the callback function without ambiguity.
// This means the params does not have to be an object, it can be anything that
// is interpreted by the CB func.
const getID = (CB = null, params = null, KVobj = null) => {
    const id = sktMsgID;
    new respMap(id, CB, params, KVobj); // Add request requirements to object
                                        // for response handling.

    sktMsgID++;
    sktMsgID %= 256; // Keeps value between 0 and 255.
    return id;
}

// Interval that runs at 1Hz. Clears all unresponded socket request.
let clearOldReq = setInterval(() => {

    const current = Date.now(); // Get current time

    // Iterate looking for socket commands that were not returned after 2 sec.
    // Clears them from the object. This prevents unanswered requests from 
    // sitting.
    Object.keys(sktID_CB).forEach(id => {

        try {
            if ((current - sktID_CB[id].timeReq) >= 2000) {
                delete sktID_CB[id];
            }
        } catch (err) {console.error(err);}

    });

}, 1000);

const handleResponse = (dev, msg) => {

    // ATTENTION. This is designed to run async based upon when a response is
    // received from the esp. When a message is sent, it generates an ID, which
    // will set any KV pair changes, and call back the function once the skt
    // responds. The typical order is send cmd, receive response. If response
    // was a change of data, instead of a poll request, all changes will be
    // reflected in the data, and then the callback function is ran which will
    // visually implement the new changes, which is designed to run out of sync
    // with the getAll polling, to reduce visual lag.
    
    let sktResp = null;

    // All messages will be sent via JSON, check for accuracy in reply and
    // parse message.
    try {
        sktResp = JSON.parse(msg.toString());

    } catch (err) {
        console.error("Invalid skt resp from device:", err);
        return;
    }

    // If valid JSON response, populate the response ID to know which callback
    // functions to run.
    sktResp.id = Number(sktResp.id); // Explicitly convert to number.
    if (isNaN(sktResp.id)) {
        console.error("Skt resp ID is NaN");
        return; // Block, will not be able to run correct function if NaN.
    }

    // This will handle the majority of non-polling cases, being change 
    // commands. Polled data like GET_ALL, will not return a status or have
    // a KV object. So if both of these exist, update the devSysData that 
    // travels with the device, before calling the callback. This will ensure
    // visual updates between polls.
    const CBchain = sktID_CB[sktResp.id];
    const KVobj = CBchain.KVobj;

    if (sktResp.status == 1 && KVobj) {
        Object.keys(KVobj).forEach(key => {
            dev.devSysData[key] = KVobj[key];
        });
    }

    // If a callback function is defined, execute the function once all data
    // has been updated to reflect changes.
    if (CBchain.CB) {
        if (CBchain.params) CBchain.CB(sktResp, CBchain.params);
        else CBchain.CB(sktResp);
    }

    delete sktID_CB[sktResp.id]; // Delete once response received.
}

module.exports = {getID, handleResponse};