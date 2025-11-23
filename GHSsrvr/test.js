const EventEmitter = require("events");

const bus = new EventEmitter();

bus.on("special-alert", (msg) => {
    console.log(msg);
});

const myMsg = "What is up";

// bus.emit("special-alert", myMsg);

const sktProm = {};
let id = 0;

const getID = (CB) => {

    sktProm[id] = new Promise((resolve, reject) => {

        const _CB = CB;

        let to = setTimeout(() => {
            reject("REJ");
        }, 5000);

        const clearTO = function() {
            clearTimeout(to);
            console.log("CLR Called");
            resolve("OK");
        }

        bus.on("special-alert", (msg) => {
            _CB();
            clearTO();
        })
    });

    sktProm[id].then(res => console.log(res)).catch(err => console.error(err));
    return id++;
}

const print = function() {
    console.log("printed");
}

let a = getID(print);

setTimeout(() => {

    bus.emit("special-alert", "msg");

}, 3000);

