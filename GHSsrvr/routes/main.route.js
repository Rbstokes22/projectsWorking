const express = require("express");
const {config} = require("../config/config");
const {getActive} = require("../controllers/main/main.controller");

const router = express.Router();

// Notes: Setup env variables like hazwx, ensure url and all that information
// is stored in it. Send the URL to the main pages as well for fetch cmd.

router.get("/", (req, res) => res.render("main", 
    {url: `${config.mDNSConfig.host}:${config.mDNSConfig.port}`}));

router.get("/getActive", getActive);

module.exports = router;