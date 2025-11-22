const {devMap} = require("../devMgr/config.devMgr");

const getAll = function(response, mdnsKey) {
    devMap[mdnsKey].devSysData = response;

    // Once data has been acquired, run all handlers to ensure information is
    // up to date.
}

module.exports = {getAll};