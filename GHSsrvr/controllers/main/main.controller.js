const {devMap} = require("../heartbeat");

const getActive = (req, res) => {
    res.json(devMap);
}

module.exports = {getActive};