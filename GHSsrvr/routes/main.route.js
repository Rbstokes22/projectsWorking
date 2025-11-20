const express = require("express");
const {getMain, getActive, getDashboard} = 
    require("../controllers/main/main.controller");

const router = express.Router();

// Notes: Setup env variables like hazwx, ensure url and all that information
// is stored in it. Send the URL to the main pages as well for fetch cmd.

router.get("/", getMain);
router.get("/getActive", getActive);
router.get("/getDashboard", getDashboard);

module.exports = router;