#ifndef WEBPAGES_HPP
#define WEBPAGES_HPP

namespace Comms {

const char WAPSetupPage[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Project Greenhouse</title>
    <style>
        body {
            background-color: rgba(0, 255, 0, 0.3);
        }
        
        h1 {
            text-align: center;
            font-weight: 900;
        }

        .p1 {
            font-size: 1.5em;
            font-weight: 700;
        }

        input {
            width: 65vw;
            height: 2em;
            border-radius: 8px;
            text-indent: 0.2em;
            font-size: 1em;
        }

        button {
            border-radius: 10px;
            padding: 0.5em;
            border-width: 3px;
        }
    </style>
</head>
<body>
    <h1>Greenhouse Network Setup</h1><br>

    <p class="p1">LAN Connection Information</p>
    <input id="ssid" placeholder="Network Name">
    <button onclick="submit('ssid')">Send</button><br><br>

    <input id="pass" placeholder="Network Password">
    <button onclick="submit('pass')">Send</button><br>

    <p class="p1">Change WAP Password</p>
    <input id="WAPpass" placeholder="WAP password">
    <button onclick="submit('WAPpass')">Send</button><br>
    
    <p class="p1">Phone (10 Digits)</p>
    <input id="phone" placeholder="Ex: 8002256776">
    <button onclick="submit('phone')">Send</button><br>

    <p class="p1">API Key (8 characters)</p>
    <input id="APIkey" placeholder="Ex: D316AA14">
    <button onclick="submit('APIkey')">Send</button><br>

    <p class="p1">Status:</p>
    <b><p id="status"></p></b>

    <script>
        // Allows the form to meet requirements before submission.
        let validate = (inputID) => {
            const temp = document.getElementById(inputID);
            const status = document.getElementById("status");
      
            // Check that input is filled out, this will work for 
            // ssid, password for the LAN. 
            if (!temp.value) {
                temp.style.border = "4px solid red";
                status.innerText = "Input cannot be subbmited blank";
                return false;
            } 

            switch (inputID) {
                case "ssid":
                    if (temp.value.length > 32) {
                        temp.style.border = "4px solid red";
                        status.innerText = "SSID must be less than 33 chars";
                        return false;

                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }

                case "pass":
                    if (temp.value.length > 64) {
                        temp.style.border = "4px solid red";
                        status.innerText = "Pass must be less than 65 chars";
                        return false;

                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }

                case "WAPpass":
                    if (temp.value.length >= 8 && temp.value.length <= 64) {
                        temp.style.border = "2px solid black";
                        return true;
                    } else {
                        status.innerText = "Password must be between 8 & 64 chars";
                        temp.style.border = "4px solid red";
                        return false;
                    }

                case "phone": 
                    // parses digits only
                    let phoneNum = temp.value.replace(/\D/g, '').toString();

                    if (phoneNum[0] === '1') {
                        phoneNum = phoneNum.substring(1);
                    }

                    if(phoneNum.length != 10) {
                        temp.style.border = "4px solid red";
                        status.innerText = "Phone Num must be 10 digits exactly";
                        return false;
                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }

                case "APIkey":
                    const re = /^([a-fA-F0-9]{8})$/;
                    if (!re.test(temp.value)) {
                        temp.style.border = "4px solid red";
                        status.innerText = "API key format is incorrect";
                        return false;
                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }
            }
        }
        
        let submit = (inputID) => {
            const temp = document.getElementById(inputID);
            const status = document.getElementById("status");

            let curURL = `${window.location.href}SubmitCreds`;
     
            if (validate(inputID)) {
                temp.style.border = "2px solid black";
                fetch(curURL, {
                method: "POST",
                headers: {"Content-Type" : "application/json"},
                body: JSON.stringify({
                    [inputID] : temp.value
                })})
                .then(response => response.json())
                .then(response => {
                    status.innerText = response.status;
                    temp.value = "";
                    if (response.status == "Accepted" || 
                        response.status == "Accepted: Reconnect to WiFi"
                    ) {
                        temp.style.border = "4px solid green";
                    }
                })
                .catch(err => {
                    status.innerText = err.message;
                });
            }
        }
    </script>
</body>
</html>
)rawliteral";

const char STApage[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Greenhouse Controls</title>
    <style>
        #title {
            text-align: center;
            font-size: 1.5em;
        }

        .sleekButton {
            text-align: center;
            background-color: red;
            color: white;
            border-radius: 5px;
            font-weight: 700;
        }
    </style>
</head>
<body onload="onLoad()">
    <p id="title">Greenhouse Monitor</p>
    <div id="otaUpd"></div>
    <div id="temp"></div>
    <div id="hum"></div>
    <div id="log"></div>
    <button onclick="openLog()">Open Log</button>

    <script>
        let socket;
        let poll;
        let checkUpdates;
        let clearReqID;

        // Network and Sockets
        const re = /(https?):\/\/([a-zA-Z0-9.-]+(:\d+)?)/;
        const URLdata = re.exec(window.location.href);
        const URLprotocol = URLdata[1]; // http or https
        const URLbody = URLdata[2]; // main url
        const OTAURL = `${URLprotocol}://${URLbody}/OTACheck`;
        const logURL = `${URLprotocol}://${URLbody}/getLog`;
        const webSktURL = `ws://${URLbody}/ws`;
        let requestIDs = {};
        let idNum = 0; // Uses to keep track of all socket commands to srvr
        let allData = {}; // This contains all sensor data from ESP32
        let log = []; // Log entries from ESP32
        const CMDS = [null, // Used to index to a +1
            "GET_ALL", "CALIBRATE_TIME", "NEW_LOG_RCVD"
        ];

        // Elements
        const OTAdisp = document.getElementById("otaUpd");

        // Intervals
        const POLL_INTV = 1000;
        const CHK_OTA_INTV = 86400000; // 24 hrs in millis
        const CLEAR_REQ_INTV = 60000;

        // Flags
        const Flags = {OTAchk:true, SKTconn:false};

        // SOCKETS
        const initWebSocket = () => {
            console.log("Initializing websocket connection");
            socket = new WebSocket(webSktURL);
            socket.onopen = socketOpen;
            socket.onclose = socketClose;
            socket.onmessage = socketMsg;

            poll = setInterval(pollServer, POLL_INTV);
            clearReqID = setInterval(clearOldRequests, CLEAR_REQ_INTV);
        }

        // Ensure the index of these match the CMDS enum from the socket header
        // In the server
        const convert = (CMD) => CMDS.indexOf(CMD);

        const isSocketOpen = () => {
            if (Flags.SKTconn && WebSocket.OPEN) {
                return true;
            }
            return false;
        }

        // Takes in the function as a parameter, and sets its id as the key
        // in requestIDs, and the passed function as the value. Returns the
        // ID to be sent to the server.
        const getID = (func = null) => {
            const id = idNum++;
            requestIDs[id] = func;
            return id;
        }

        // Polls the server at a set interval requesting all data.
        const pollServer = () => {
            if (!isSocketOpen()) return;
            const ID = getID(setAll);
            socket.send(`${convert("GET_ALL")}/${0x00}/${ID}`);
        }

        // Clears requests that will not be satisfied by the server to 
        // prevent the from accumulating to non responses.
        const clearOldRequests = () => {
            const curID = idNum;

            Object.keys(requestIDs).forEach(id => {
                if (Number(id) < curID - 5) {
                    delete requestIDs[id];
                }
            });
        }

        // Gets the log, separates it into a non-delimited array, to be used
        // for diplay.
        let getLog = () => {
            fetch(logURL)
            .then(response => {
                if (!response.ok) {
                    throw new Error(`HTTP error status: ${response.status}`);
                }
                return response.text();
            })
            .then(text => {
                console.log(text);

                if (text.length <= 0) {
                    throw new Error("No Log Data");
                }

                log = text.split(";");

                const ID = getID();
                socket.send(`${convert("NEW_LOG_RCVD")}/${0x00}/${ID}`);
            })
            .catch(err => console.log(err)); // Handle later
        }

        const openLog = () => {
            let html = "";
            log.forEach(entry => html += `${entry}<br>`);
            document.getElementById("log").innerHTML = html;
        }

        let calibrateTime = (seconds) => { // calibrates time if different
            const time = new Date();
            let secPastMid = (time.getHours() * 3600) + (time.getMinutes() * 60) 
                + time.getSeconds();

            if (seconds != secPastMid) { // Calibrates clock if unequal
                const ID = getID();
                socket.send(`${convert("CALIBRATE_TIME")}/${secPastMid}/${ID}`);
            }
        }

        // RECEIVED MESSAGE HANDLERS
        const setAll = (data) => { // Sets the addData object to response
            allData = data; // Allows modification between poll interval waits
            const title = document.getElementById("title");
         
            if (data.newLog === 1) getLog(); // Gets log if avail
            calibrateTime(data.sysTime); // Ensures sys clock is calib to client

            title.innerHTML = `Greenhouse Monitor V${data.firmv}`;
       
        }

        // Takes the JSON response, and executes the function
        const handleResponse = (response) => {
            const id = response.id;
            let func = requestIDs[id];
   
            if (func != null) {
                func(response);
            } 

            delete requestIDs[id];
        }

        // SOCKET MANAGERS

        const socketOpen = () => {
            console.log("Connected to server");
            Flags.SKTconn = true;
        }

        const socketClose = () => {
            console.log("Disconnected from server");
            Flags.SKTconn = false;
            clearInterval(poll);
            clearInterval(clearReqID);
            setTimeout(initWebSocket, 2000);
        }

        const socketMsg = (event) => {
            const response = JSON.parse(event.data);
            handleResponse(response);  
        }

        // HTTP 
        const checkOTA = () => {
            return new Promise((resolve, reject) => {
            fetch(OTAURL)
            .then(res => res.json())
            .then(res => {
                const version = res.version;
                const noActionResp = ["Invalid JSON", "match"];

                if (noActionResp.indexOf(version) == -1) {
                    const html = `
                        <button class="sleekButton" onclick="DLfirmware(
                        '${res.signatureURL}', '${res.firmwareURL}')">
                            Update to Version ${version}
                        </button>
                    `;

                    OTAdisp.innerHTML = html;
                    resolve(200);
                }
            })
            .catch(err => reject(err));
        });
        }

        const DLfirmware = (sigURL, firURL) => {
            const URL = `${URLprotocol}://${URLbody}/OTAUpdate?url=${firURL}&sigurl=${sigURL}`;
            let updHTML = OTAdisp.innerHTML;

            fetch(URL)
            .then(resp => resp.text())
            .then(resp => {
                if (resp === "OK") {
                    Flags.OTAchk = true; // should restart anyway, redundant
                    let secToReload = 10;
                    let intervalID = setInterval(() => {
                        OTAdisp.innerText = `Restarting in ${secToReload}`;
                        secToReload--;
                        if (secToReload < 1) {
                            clearInterval(intervalID);
                            window.location.reload();
                        }
                    }, 1000);
                } else {
                    updHTML += " (Failed)";
                    OTAdisp.innerHTML = updHTML;
                }
            })
            .catch(err => {
                updHTML += ` (Error)`;
                OTAdisp.innerHTML = updHTML;
            });
        }

        checkUpdates = setInterval(() => {
            const lastCheck = Number(localStorage.getItem("OTA"));
            const curTime = Date.now();

            const runCheck = () => {
                if (Flags.OTAchk) {
                    checkOTA()
                    .then(resp => {
                        localStorage.setItem("OTA", curTime);
                        Flags.OTAchk = false;
                    });
                }  
            }

            if (lastCheck === null) { // First start
                runCheck();
            } else {
                const expireTime = lastCheck + CHK_OTA_INTV;
                if (curTime >= expireTime) runCheck();
            }

        }, 600000); // Checks every 10 min

        setTimeout(() => { // checkOTA upon start + 15 sec
            checkOTA()
            .then(resp => {
                localStorage.setItem("OTA", Date.now());
                Flags.OTAchk = false;
            });
        }, 15000);

        let onLoad = () => {
            initWebSocket();
            getLog(); // Gets the log when loading the page.
        }

    </script>
    
</body>
</html>
)rawliteral";

}

#endif // WEBPAGES_HPP