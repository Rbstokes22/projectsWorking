const { clearTimeout } = require("timers");
const {config} = require("../../config/config");
const {devMap} = require("../devMgr/config.devMgr");
const EventEmitter = require("events");
const bus = new EventEmitter();

let sktMsgID = 0; // Will be sent with each socket command to allow async 
                  // returns to know which callback to call. Must be between
                  // 0 and 255. Handled by getID().

// Requires the callback function, parameters, and key value object. This is 
// set in every promise in getID(). These values can be null, and will be set
// to null by default in the calling function if not passed. This data will
// be executed upon socket response.
const respMap = function(CB, params, KVobj) {
    this.CB = CB;
    this.params = params;
    this.KVobj = KVobj;

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
// is interpreted by the assigned CB func.
const getID = (CB = null, params = null, KVobj = null) => {
    const id = sktMsgID; // Used as the object key.
                   
    // Create a new promise that will be resolved once the socket responds.
    // This does not guarantee follow on, just response, and will have the 
    // timeout cleared upon response.
    const prom = new Promise((resolve, reject) => {

        // Callback response map. Will be used to run callbacks and change 
        // K/V pairs if required.
        const rMap = new respMap(CB, params, KVobj); 
        const reqID = id;

        // Starts timeout upon creation of promise. Always rejects, unless
        // event is emitted clearing the timeout to resolve the promise.
        let respTimeout = setTimeout(() => {

            // Unregister the listener before reject.
            bus.off(config.SKT_ASYNC_BUS_EVENT, handler);
            reject("SKT RESP TIMEOUT");

        }, config.SKT_TIMEOUT);

        // Handles the emission message passed by the handleResponse() func.
        const handler = (respOb) => {
           
            // Indicates that the response is meant for this promise handler.
            if (respOb.id === reqID) {

                // Unregister listener upon successful listen and clear T/O.
                clearTimeout(respTimeout);
                bus.off(config.SKT_ASYNC_BUS_EVENT, handler);

                if (respOb.status === 1) { // Success, run callbacks.

                    handleCallbacks(rMap, respOb);
                    resolve("OK");
                 
                } else { // Failure.
                    reject("SKT STATUS FAIL");
                }
            }
        }

        bus.on(config.SKT_ASYNC_BUS_EVENT, handler); // Enable listener.
    });

    sktMsgID++;
    sktMsgID %= 256; // Keeps value between 0 and 255.

    // Return the id for use, and the promise for chain handling.
    return {"id": id, "promise": prom};
}

// Requires the response Map which contains callbacks, params, and key/val 
// pairs. Also requires the response Object generated in handleResponse(),
// that is emmitted to the custom promise event listener. Updates all required
// data and runs all callbacks. This function is only called upon a successful
// return from the esp.
const handleCallbacks = (responseMap, responseOb) => {

    // Checks if any key value objects were passed. Updates the device's
    // system data which is retrieved from the esp. This will allow the visible
    // data on the dashboard to reflect change if a change command is called
    // between polls. Instead of making a change and waiting up to 1 second for
    // the changes to reflect, this will update immediately upon receiving a
    // status = 1 from the esp.
    if (responseMap.KVobj) {
        Object.keys(KVobj).forEach(key => {
            devMap[responseOb.mdns].devSysData[key] = KVobj[key];
        });
    }

    // If a callback exists, runs the callback with the updated data. If 
    // params are defined, they will be passed as the second param to the 
    // callback. 
    if (responseMap.CB) {
        if (responseMap.params) responseMap.CB(responseOb.response, 
            responseMap.params);

        else responseMap.CB(responseOb.response);
    }
}

// Requires the mdns, which is the key value used in devMap, to allow device
// lookup, as well as the json message returned by the esp.
const handleResponse = (mdnsKey, msg) => {

    // ATTENTION. The esp uses Async sockets, and this function is designed 
    // to run async as well. When a message is sent to the esp, it includes
    // an ID. Upon receipt of that ID, an event message is generated using
    // the response status (1 = OK, 0 = FAIL), the message serial ID,
    // the devices mdns, which is the key in devMap, for data access, and 
    // the socket response. It is then emitted to all active promises with
    // listeners. The listener will ensure its ID === the response id, and 
    // handle it appropriately.
    
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

    // Not all socket responses from the esp include a status, only those that
    // require a change. Polling data does not. This allows a status to default
    // to 1 if it is not included, bypassing future checks.
    const respStat = sktResp.status ? sktResp.status : 1;

    // Emit a signal with the status and ID to allow the promise to resolve 
    // or reject, once callbacks are executed.
    const eventMsg = {"status": respStat, "id": sktResp.id, "mdns": mdnsKey,
        "response": sktResp
    };

    bus.emit(config.SKT_ASYNC_BUS_EVENT, eventMsg);
}

module.exports = {getID, handleResponse};