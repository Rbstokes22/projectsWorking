const {devMap} = require("../config.devMgr");
const {handleAverages} = require("../averages/averagesTrends");

const getAll = function(response, mdnsKey) {
    devMap[mdnsKey].devSysData = response;

    // Once data has been acquired, run all handlers to ensure information is
    // up to date.
    handleAverages(mdnsKey);
}





module.exports = {getAll};