const {devMap} = require("../devMgr/config.devMgr");
const {config} = require("../../config/config");
const URL = `${config.mDNSConfig.host}:${config.mDNSConfig.port}`;

const getMain = (req, res) => res.render("main", {url: URL});
const getActive = (req, res) => {

    // Extract the required info for the client. Do not include the entire
    // devMap as that contains too much data and will cause cirular issues.
    const data = {};

    Object.keys(devMap).forEach(mdns => {

        const {name, isUp, ip, sockURL} = devMap[mdns];

        data[mdns] = {
            name: name,
            isUp: isUp,
            ip: ip,
            sockURL: sockURL
        }
    });

    res.json(data);
}

const getDashboard = (req, res) => {
    const {ip, ws} = req.query;
   
    res.render("dashboard", {url: URL, ip: ip, ws: ws});
}

module.exports = {getMain, getActive, getDashboard};