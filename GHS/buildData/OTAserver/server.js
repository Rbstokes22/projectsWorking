const express = require("express");
const os = require("os");

const app = express();

const FWpath = "/home/shadyside/Desktop/Programming/projects/GHS/.pio/build/esp32dev/firmware.bin";
const SIGpath = "/home/shadyside/Desktop/Programming/projects/GHS/data/app0firmware.sig";
const PORT = 5702;

let getIP = () => {
    const interfaces = os.networkInterfaces();
    let ipaddr;

    for (const ifName in interfaces) {
        for (const iface of interfaces[ifName]) {

            if (iface.family === 'IPv4' && !iface.internal) {
                ipaddr = iface.address;
                break;
            }
        }
        if (ipaddr) break;
    }
    return ipaddr;
}

app.use((err, req, res, next) => {
    console.error("Global Error Handler: ", err);
    res.status(500).send("Internal Server Error");
})

// Meant to show connection
app.get("/", (req, res) => {
    res.status(200).send("Success");
});

app.get("/IP", (req, res) => {
    res.status(200).send(getIP());
});

app.get("/sigUpdate", (req, res) => {
    res.setHeader("Content-Type", "application/octet-stream");
    console.log("Called Signature File");

    res.sendFile(SIGpath, (err) => {
        if (err) {
            if (res.headersSent) {
                console.error("Response already sent: ", err);
            } else {
                console.error("Error sending Sig, ", err);
            }
        }
    });
});

app.get("/FWUpdate", (req, res) => {
    res.setHeader("Content-Type", "application/octet-stream");
    console.log("Called Firmware File");

    res.sendFile(FWpath, (err) => {
        if (err) {
            if (res.headersSent) {
                console.error("Response already sent: ", err);
            } else {
                console.error("Error sending Firmware, ", err);
            }
        } else {

        }
    });
});

app.listen(PORT, () => {
    console.log(`
        To update device use URL: 
        http://greenhouse.local/OTAUpdateLAN?url=http://${getIP()}:${PORT}
        `);
});