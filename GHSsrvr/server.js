const { config } = require("./config/config");
const { resolveRAMPath, startmDNS } = require("./startup/startup");
const express = require("express");
const cors = require("cors");

const app = express(); // Create express app.

resolveRAMPath(); // Sets the config object RAM_PATH to path.
startmDNS();

const mainRoute = require("./routes/main.route");

// set the static file paths served
app.use("/css", express.static(`${__dirname}/css`));
app.use("/clientJS", express.static(`${__dirname}/clientJS`));
app.use("/images", express.static(`${__dirname}/images`));

// set url json and cors.
app.use(express.urlencoded({extended: true}));
app.use(express.json());
app.use(cors({
    origin: config.WEB_PATH, // Allow outside access from our site only.
    methods: ["GET", "POST", "PUT", "DELETE"],
    credentials: true // Allow cookies and auth headers if needed.
}));

// Set viewing.
app.set("view engine", "ejs");
app.set("views", `${__dirname}/views`);

// Set routes
app.use("/", mainRoute);








app.listen(config.mDNSConfig.port, () => {
    console.log(
        `Running Server @ ${config.mDNSConfig.host}:${config.mDNSConfig.port}`);
});

