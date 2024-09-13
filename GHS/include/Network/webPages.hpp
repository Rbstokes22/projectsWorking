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
    <h1>Project Greenhouse Network Setup</h1><br>

    <p class="p1">Required for LAN</p>
    <input id="ssid" placeholder="Network Name">
    <button onclick="submit('ssid')">Send</button><br><br>

    <input id="pass" placeholder="Network Password">
    <button onclick="submit('pass')">Send</button><br>

    <p class="p1">Change WAP Password</p>
    <input id="WAPpass" placeholder="WAP password">
    <button onclick="submit('WAPpass')">Send</button><br>
    
    <p class="p1">Optional: (10 digit phone)</p>
    <input id="phone" placeholder="8002256776">
    <button onclick="submit('phone')">Send</button><br>

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
                    if (temp.value.length > 31) {
                        temp.style.border = "4px solid red";
                        status.innerText = "SSID must be less than 32 chars";
                        return false;

                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }

                case "pass":
                    if (temp.value.length > 63) {
                        temp.style.border = "4px solid red";
                        status.innerText = "Pass must be less than 64 chars";
                        return false;

                    } else {
                        temp.style.border = "2px solid black";
                        return true;
                    }

                case "WAPpass":
                    if (temp.value.length >= 8 && temp.value.length <= 63) {
                        temp.style.border = "2px solid black";
                        return true;
                    } else {
                        status.innerText = "Password must be between 8 & 63 chars";
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
                        response.status == "Accepted, Reconnect to WiFi"
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

    <script>
        let socket;
        let poll;
        let checkUpdates;

        // Network and Sockets
        const re = /([a-zA-Z]+\.[a-zA-Z]+)|(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})/;
        const URLonly = re.exec(window.location.href);
        const WSPORT = 8080;
        let requestIDs = {};
        let Queue = {};
        let idNum = 0;
        const CMDS = ["GET_VERSION", "GET_TEMP", "GET_HUM"];

        // Elements
        const OTAdisp = document.getElementById("otaUpd");

        // Intervals
        const POLL_INTV = 500;
        const CHK_OTA_INTV = 86400000; // 24 hrs in millis

        // Flags
        const Flags = {OTAchk:true, SKTopen:false};

        // SOCKETS
        const initWebSocket = () => {
            console.log("Initializing websocket connection");
            socket = new WebSocket(`ws://${URLonly[0]}:${WSPORT}`);
            socket.onopen = socketOpen;
            socket.onclose = socketClose;
            socket.onmessage = socketMsg;

            poll = setInterval(pollServer, POLL_INTV);
        }

        // Ensure the index of these match the CMDS enum from the socket header
        // In the server
        const convert = (CMD) => CMDS.indexOf(CMD);

        // Takes in the function as a parameter, and sets the function to
        // and id generated by the timestamp. This is used by the server to 
        // respond with the data to the associated id, in the event the server
        // responds to a latter request before a former.
        const getID = (func = null) => {
            const id = idNum++;
            requestIDs[id] = func;
            return id;
        }

        const pollServer = () => {
            if (!Flags.SKTopen) return;
            const setFW_ID = getID(setFW);
            const setTemp_ID = getID(setTemp);
            const setHum_ID = getID(setHum);

            Queue[setFW_ID] = `${convert("GET_VERSION")}/${0x00}/${setFW_ID}`;
            Queue[setTemp_ID] = `${convert("GET_TEMP")}/${0x00}/${setTemp_ID}`;
            Queue[setHum_ID] = `${convert("GET_HUM")}/${0x00}/${setHum_ID}`;
        }

        const sendMess = setInterval(() => {
            const key = Object.keys(Queue)[0];
            if (key && socket.readyState === WebSocket.OPEN) {
                socket.send(Queue[key]);
                delete Queue[key];
            } else if (socket.readyState != WebSocket.OPEN) {
                console.warn(`Socket state: ${socket.readyState}`);
            } 
        }, 5);

        const setFW = (version) => {
            document.getElementById("title").innerHTML = `Greenhouse Monitor V${version}`;
        }

        const setTemp = (temp) => {
            document.getElementById("temp").innerText = `Temp = ${temp}`;
        }

        const setHum = (hum) => {
            document.getElementById("hum").innerText = `Hum = ${hum}`;
        }

        const handleResponse = (response) => {
            const data = response[0];
            const id = response[1];
        
            let func = requestIDs[id];
            if (func != null) {
                func(data);
            } 

            delete requestIDs[id];
        }

        const socketOpen = () => {
            console.log("Connected to server");
            Flags.SKTopen = true;
        }

        const socketClose = () => {
            console.log("Disconnected from server");
            Flags.SKTopen = false;
            clearInterval(poll);
            setTimeout(initWebSocket, 2000);
        }

        const socketMsg = (event) => {
            const response = event.data.toString();
            console.log(response);
            handleResponse(response.split('/'));  
        }

        // HTTP 
        const checkOTA = () => {
            return new Promise((resolve, reject) => {
            const OTAURL = `${window.location.href}OTACheck`;
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
            const URL = `${window.location.href}OTAUpdate?url=${firURL}&sigurl=${sigURL}`;
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
        }, 60000); // Checks every min

        setTimeout(() => { // checkOTA upon start + 15 sec
            checkOTA()
            .then(resp => {
                localStorage.setItem("OTA", Date.now());
                Flags.OTAchk = false;
            });
        }, 15000);

        let onLoad = () => {
            initWebSocket();
        }

    </script>
    
</body>
</html>
)rawliteral";

}

#endif // WEBPAGES_HPP