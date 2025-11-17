const {resolveRAMPath} = require("./startup/startup");
const express = require("express");
const cors = require("cors");
const { devMap } = require("./controllers/heartbeat");
const Bonjour = require("bonjour-service").default; // mDNS
const bonjour = new Bonjour();

const RAM_PATH = resolveRAMPath(); // Gets the RAMDISK path.
const WEB_PATH = "https://www.mysterygraph.org"; // PWA site.
const mDNSConfig = { // MDNS will be broadcast with this info.
    name: "ghmain",
    type: "http",
    port: 5702,
    host: "ghmain.local",
    txt: {version: "1.0"}
};

const app = express(); // Create express app.

bonjour.publish(mDNSConfig); 

const mainRoute = require("./routes/main.route");

app.set("view engine", "ejs");
app.set("views", `${__dirname}/views`);
app.use(cors({
    origin: WEB_PATH, // Allow outside access from our site only.
    methods: ["GET", "POST", "PUT", "DELETE"],
    credentials: true // Allow cookies and auth headers if needed.
}));

app.use("/", mainRoute);








app.listen(mDNSConfig.port, () => {
    console.log(`Running Server @ ${mDNSConfig.host}:${mDNSConfig.port}`);
});

process.on("SIGINT", () => {
    console.log("Unpublishing mDNS services and shutting down");
    bonjour.unpublishAll(() => {
        bonjour.destroy();
        process.exit();
    });
});