#ifndef WEBPAGES_HPP
#define WEBPAGES_HPP

namespace Comms {

// Has credential information to connect to station and utilize sms.
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

// Main station page. Has the most functionality.
const char STApage[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Greenhouse Controls</title>
    <style>
        
        button {
            border: 2px solid black;
            border-radius: 5px;
        }

        #title {
            text-align: center;
            font-size: 1.5em;
        }

        .sleekButton {
            text-align: center;
            background-color: black;
            color: white;
            border-radius: 5px;
            font-weight: 700;
        }

        #FWbut {background-color: red;} 

        /* 
        Creates as many col that will fit into container, if there is space
        will fill row with as many col as possible. Each col will have a min
        wid of 200px and a max of 1 fraction of available space, allowing
        col to grow to fill available space  
        */
        #container { 
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 10px;
        }

        .item {
            background-color: lightblue;
            padding: 20px;
            text-align: left;
            border: 2px solid black;
        }

        .smallItem {
            padding: 5px;
            border: 2px solid black;
            text-align: left;
        }

        #tempCon, #humCon, #soil1Con, #soil2Con, #soil3Con, #soil4Con,
        #lightCon {
            display: grid;
            grid-template-columns: repeat(4, minmax(75px, 1fr));
            grid-template-rows: auto;
            gap: 10px;
        }

        .span2 {grid-column: span 2;}
        .span3 {grid-column: span 3;}
        .span4 {grid-column: span 4;}
        .selected {background-color: yellow;}
        .boundBust {background-color: red;}

    </style>
</head>
<body onload="loadPage()">
    <p id="title">Greenhouse Monitor</p>
    <div id="otaUpd"></div>
    <div id="timeCal"></div>

    <div id="container">
        <div id="tempCon" class="item"></div>
        <div id="humCon" class="item"></div>
        <div id="soil1Con" class="item"></div>
        <div id="soil2Con" class="item"></div>
        <div id="soil3Con" class="item"></div>
        <div id="soil4Con" class="item"></div>
        <div id="lightCon" class="item"></div>
        <div id="specCon" class="item"></div>
        <div id="re1Con" class="item">RE1</div>
        <div id="re2Con" class="item">RE2</div>
        <div id="re3Con" class="item">RE3</div>
        <div id="re4Con" class="item">RE4</div>
    </div> <!-- contains all periph containers-->
    
    <div id="log"></div>

    <button id="logBut" class="sleekButton" onclick="openLog()">Open Log</button>

    <script>

        // (I THINK I AM DONE WITH THE SOCKET SEND PART.)
        // Currently working temperature. Skt command to server should be the
        // set value * 100. So if you are setting a temp for a relay of 10.2C,
        // it should be sent as 1020 via skt. Ensure that step size is 0.1 when
        // using celcius as well, this will be a global setting that can be 
        // changed. When reading, the temp will be float val, but the re/alt vals
        // will be int * 100, so this must be divided for display purpose. Ensure
        // that if in faren, that is accounted for as well. The important thing to
        // remember is that all communication is in celcius and must be converted
        // when sending and when reading. So if using F at 68 degrees, it should
        // convert the value to 20 C, which will be multiplied by 100 to be 2000.
        // When reading this same value, it should read 2000, and convert it back
        // to 68 for reading. Ensure that the container builder logic has specific
        // stuff about temp only, since units apply no where else.

        // Work on light, and then add relays. Reminder that the spec data
        // will be RO, with no interaction, and can probably be a graph.

        // element build. Requires the parent ID, and both the read only and 
        // input objects. Returns constructed object.
        let eleBuild = function(parentID, readOnly, inputs) {
            this.parentID = parentID; // Used to link it to parent container.
            this.readOnly = readOnly; // Sensor and setting data.
            this.inputs = inputs; // Interactive.
            this.buttonID = `${parentID}Exp`; // Expansion button.
            return this;
        }

        // Read only build. Requires the element type, its text, and and array
        // of all classes pertaining to it. Returns constructed object.
        let RObuild = function(eleType, text, classes) {
            this.eleType = eleType; // element type. i.e. "div".
            this.text = text; // Default input text.
            this.classes = classes; // array of classes.
            return this;
        }

        // Input build. Builds the interactive portion to make adjustments to
        // system settings. Pass null if non-exist. Returns constructed object.
        let inputBuild = function(min, max, step, eleType, label, incBut, 
            classes, dataPtr, rangeMap, specMap, butFunc) {

            this.min = min; // range min
            this.max = max; // range max
            this.step = step; // step size of range bar.
            this.eleType = eleType; // element type. i.e. "input".
            this.label = label; // input box label.
            this.incBut = incBut; // bool. Include button.
            this.classes = classes; // array of classes.
            this.dataPtr = dataPtr; // pointer/key of allData JSON.

            // Maps. The range map is an array that has display values with an
            // index that corresponds with an actual numberical value. Special
            // maps are used for values outside of the typical range and are 
            // passed as objects.
            this.rangeMap = rangeMap; 
            this.specMap = specMap;
            this.butFunc = butFunc; // function applied to button onclick.
            return this;
        }

        // Command build. Pass the command string, and the callback function. 
        // Returns constructed object that will be called when submit is sel.
        let cmdBuild = function(cmd, callBack) {
            this.cmd = cmd;
            this.callBack = callBack;
            return this;
        }

        // Build container. This is a sort of constructor that takes the name,
        // relay command, and both the relay and alert setting commands. Pass
        // null if non-exist. Can be used for the majority of containers since
        // the naming convention is cookie-cutter. Builds and returns container.
        let buildCon = (name, re, reSet, altSet) => {
            let RO = {}, INP = {}; // Read only and Input.

            // ATTENTION: Read only will consist of data reported by ESP. It
            // will show for example, temperature, relay, and alert settings.
            // Input will allow those settings to be changed by signalling srvr.

            // Change temperature step size ONLY depending on units.
            const step = (name === "temp" && isCelcius) ? 0.2 : 1;

            RO[name] = new RObuild("div", "0", ["smallItem", "span4"]); // Pri

            if (re) { // Relay selection. If exists, builds boxes.
                RO[`${name}Re`] = new RObuild("div", "0", ["smallItem", "span4"]);

                INP[`${name}ReAtt`] = new inputBuild(0, 4, 1, "input", "Plug", true, 
                    ["smallItem"], `${name}Re`, ['1', '2', '3', '4', "NONE"],
                    {255: "NONE"}, () => {
                        const CMD = convert(re.cmd);
                        const reNum = document.getElementById(`${name}ReAttHidden`).value;



                        // Since a rangeMap is attached, a hidden value will be
                        // built to hold the true value, vs display value.
                        
                        const disp = (reNum == 4 ) ? RE_OFF : reNum;
                        socket.send(`${convert(re.cmd)}/${reNum}/` +
                            `${getID(defResp, {[`${name}Re`]: disp}, re.callBack)}`);
                    });
            }

            if (reSet) {
                INP[`${name}ReLTGT`] = new inputBuild(0, 2, 1, "input", "Plug Cond",
                    false, ["smallItem"], `${name}ReCond`, 
                    ["Less Than", "Gtr Than", "NONE"], null, null);

                INP[`${name}ReSetAdj`] = new inputBuild(-30, 120, step, "input", `Plug ${name}`, 
                    true, ["smallItem"], `${name}ReVal`, null, null, () => {
                        const cond = document.getElementById(`${name}ReLTGTHidden`).value;
                        let val = document.getElementById(`${name}ReSetAdj`).value;
                        const cmd = convert(reSet.cmd) + Number(cond);

                        if (name === "temp") { // Adjust based on requirements
                            val = isCelcius ? val * 100 : 
                                (((val - 32) / 1.8).toFixed(2) * 100);

                            
                        }

                        const ID = getID(defResp, 
                            {[`${name}ReCond`]: cond, [`${name}ReVal`]: val}, 
                            reSet.callBack);
                        socket.send(`${cmd}/${val}/${ID}`);
                    });
            }

            if (altSet) {
                RO[`${name}Alt`] = new RObuild("div", "0", ["smallItem", "span4"]);

                INP[`${name}AltLTGT`] = new inputBuild(0, 2, 1, "input", "Alert Cond", 
                    false, ["smallItem"], `${name}AltCond`, 
                    ["Less Than", "Gtr Than", "NONE"], null, null);

                INP[`${name}AltSetAdj`] = new inputBuild(-30, 120, step, "input", 
                    `Alert ${name}`, true, ["smallItem"], `${name}AltVal`, null, 
                    null, () => {
                    const cond = document.getElementById(`${name}AltLTGTHidden`).value;
                    let val = document.getElementById(`${name}AltSetAdj`).value;
                    const cmd = convert(altSet.cmd) + Number(cond);

                    if (name === "temp") { // Adjust based on requirements
                            val = isCelcius ? val * 100 : 
                                (((val - 32) / 1.8).toFixed(2) * 100);
                    }

                    const ID = getID(defResp, 
                        {[`${name}AltCond`]: cond, [`${name}AltVal`]: val}, 
                        altSet.callBack);
                    socket.send(`${cmd}/${val}/${ID}`);
                    });
            }

            RO[`${name}ConExp`] = new RObuild("button", "Expand", ["span4"]);
            INP[`${name}ConAdj`] = new inputBuild(0, 0, 0, "div", "Adjust", 
                false, ["smallItem", "span3"], null, null, null);

            return new eleBuild(`${name}Con`, RO, INP);
        }

        // All commands from socketHandler.hpp. These must correspond with the
        // enum. Set null as pos 0 to allow GET_ALL to be pos 1, or cmd 1.
        const CMDS = [null, "GET_ALL", "CALIBRATE_TIME", "NEW_LOG_RCVD",
            "RELAY_1", "RELAY_2", "RELAY_3", "RELAY_4",
            "RELAY_1_TIMER_ON", "RELAY_1_TIMER_OFF", 
            "RELAY_2_TIMER_ON", "RELAY_2_TIMER_OFF",
            "RELAY_3_TIMER_ON", "RELAY_3_TIMER_OFF",
            "RELAY_4_TIMER_ON", "RELAY_4_TIMER_OFF",
            "ATTACH_TEMP_RELAY", "SET_TEMP_RE_LWR_THAN", "SET_TEMP_RE_GTR_THAN",
            "SET_TEMP_RE_COND_NONE", "SET_TEMP_ALT_LWR_THAN", "SET_TEMP_ALT_GTR_THAN",
            "SET_TEMP_ALT_COND_NONE",
            "ATTACH_HUM_RELAY", "SET_HUM_RE_LWR_THAN", "SET_HUM_RE_GTR_THAN", 
            "SET_HUM_RE_COND_NONE", "SET_HUM_ALT_LWR_THAN", "SET_HUM_ALT_GTR_THAN",
            "SET_HUM_ALT_COND_NONE", "CLEAR_TEMPHUM_AVG",
            "SET_SOIL1_ALT_LWR_THAN", "SET_SOIL1_ALT_GTR_THAN", "SET_SOIL1_ALT_COND_NONE",
            "SET_SOIL2_ALT_LWR_THAN", "SET_SOIL2_ALT_GTR_THAN", "SET_SOIL2_ALT_COND_NONE",
            "SET_SOIL3_ALT_LWR_THAN", "SET_SOIL3_ALT_GTR_THAN", "SET_SOIL3_ALT_COND_NONE",
            "SET_SOIL4_ALT_LWR_THAN", "SET_SOIL4_ALT_GTR_THAN", "SET_SOIL4_ALT_COND_NONE",
            "ATTACH_LIGHT_RELAY", "SET_LIGHT_RE_LWR_THAN", "SET_LIGHT_RE_GTR_THAN",
            "SET_LIGHT_RE_COND_NONE", "SET_DARK_VALUE", "CLEAR_LIGHT_AVG",
            "SEND_REPORT_SET_TIME", "SAVE_AND_RESTART", "GET_TRENDS"];

        // Declare all vars here, to save space. idNum is used to keep track of
        // socket commands, allData contains all sensor data, and log contains
        // all log entries.
        let socket, poll, clearReqID, requestIDs = {}, idNum = 0,
            allData = {}, log = [], trends = {}, Expansions = {};

        // Network and Sockets
        const re = /(https?):\/\/([a-zA-Z0-9.-]+(:\d+)?)/; // Regex
        const URLdata = re.exec(window.location.href);
        const URLprotocol = URLdata[1]; // http or https
        const URLbody = URLdata[2]; // main url
        const OTAURL = `${URLprotocol}://${URLbody}/OTACheck`; 
        const logURL = `${URLprotocol}://${URLbody}/getLog`;
        const webSktURL = `ws://${URLbody}/ws`;
    
        // Elements
        const OTAdisp = document.getElementById("otaUpd");

        // Intervals (millis)
        const POLL_INTV = 1000; // Poll interval to run GET_ALL.
        const CHK_OTA_INTV = 86400000; // OTA check run.
        const CLEAR_REQ_INTV = 60000; // Clear exp skt req if non-response.
        const FW_CHECK_INTV = 12 * 60 * 60 * 1000; // Check for new firmware.

        // Flags 
        let Flags = {SKTconn:false, openLog:true};

        // Other
        const logDelim = ';';
        const RE_OFF = 255; // Signals relay is not attached.
        let isCelcius = false;

        // Returns the index of the command, this is used with esp32 ssocket
        // command as argument 1.
        const convert = (CMD) => CMDS.indexOf(CMD);

        // Requires callback function, html id, and value if successful. Sets
        // the key to the ID, and a 3 element array to the function, html ele
        // id, and value to change if successful. This allows a successful
        // sys change to update between polling.
        const getID = (func = null, KVs = null, func2 = null) => {
            const id = idNum++;
            requestIDs[id] = [func, KVs, func2]; 
            return id;
        }

        // No params. Returns true if socket is open, false if not.
        const isSocketOpen = () => (Flags.SKTconn && WebSocket.OPEN);

        // SOCKETS. Event handlers.
        const initWebSocket = () => {
            console.log("Init websocket");
            socket = new WebSocket(webSktURL); // Open new socket.
            socket.onopen = socketOpen; // Set the event handler functions.
            socket.onclose = socketClose;
            socket.onmessage = socketMsg;
        }

        const socketOpen = () => { // Open socket handler
            console.log("Connected to server");
            Flags.SKTconn = true;
            poll = setInterval(pollServer, POLL_INTV); // Set intervals
            clearReqID = setInterval(clearOldRequests, 10000);
        }

        const socketClose = () => { // Close socket handler
            console.log("Disconnected from server");
            Flags.SKTconn = false;
            clearInterval(poll); // Clear intervals.
            clearInterval(clearReqID);
            setTimeout(initWebSocket, 2000); // Attempt reconnect.
        }

        // Messages from server will be in JSON format, parsed, and sent to the
        // assigned function to handle that response.
        const socketMsg = (event) => handleResponse(JSON.parse(event.data)); 

        // Receives the parsed JSON response from the socket message. Gets its
        // function from the ID, runs the function passing the response, and
        // deletes the ID signifying the request is complete.
        const handleResponse = (response) => {
            const func = requestIDs[response.id][0]; // callback function
      
            // Check if default response. If so, will send the html element
            // ID and the value to set it to iff successful. This will handle
            // most socket responses.
            if (func === defResp && response.status === 1) { // 1 = success
                func(requestIDs[response.id][1], requestIDs[response.id][2]);
            } else if (func != null) { 
                func(response); // Used moreso for trends and get all.
            }

            delete requestIDs[response.id]; // Delete corresponding ID.
        }
    
        // Polls the server at a set interval requesting all data.
        const pollServer = () => {
            if (isSocketOpen()) {
                socket.send(`${convert("GET_ALL")}/${0x00}/${getID(getAll)}`);
            }
        }

        // Clears requests that will not be satisfied by the server to 
        // prevent the from accumulating to non responses.
        const clearOldRequests = () => {
            Object.keys(requestIDs).forEach(id => {
                if (Number(id) < (idNum - 3)) delete requestIDs[id];
            });
        }

        // Gets the log, separates it into a non-delimited array, to be used
        // for diplay.
        let getLog = () => { // HTTP call
            const button = document.getElementById("logBut");
            fetch(logURL)
            .then(response => {
                if (!response.ok) {
                    throw new Error(`HTTP error status: ${response.status}`);
                }
                return response.text(); // If no err, proceed.
            })
            .then(text => {
                if (text.length <= 0) throw new Error("No Log Data");
                log = text.split(logDelim); // Split by delim into array.
                button.style.backgroundColor = "green"; // Shows new log

                // Is a receipt only, Allows server to remove flag.
                if (!isSocketOpen()) return; // Block
                socket.send(`${convert("NEW_LOG_RCVD")}/${0x00}/${getID()}`); 
            })
            .catch(err => console.log(err));
        }

        const openLog = () => { // Opens the log for display.
            const button = document.getElementById("logBut");
            const logDisp = document.getElementById("log");
   
            if (Flags.openLog) {
                let html = "";
                log.forEach(entry => html += `${entry}<br>`);
                logDisp.innerHTML = html;
                button.innerText = "Close log";
                Flags.openLog = false;

            } else if (!Flags.openLog) {
                logDisp.innerText = ""; // Clear out.
                button.style.backgroundColor = "black";
                button.innerText = "Open log";
                Flags.openLog = true;
            }
        }

        // Qty 1 - 12. Default to 6. Gets the previous n hours of temp/hum or
        // light trends.
        let getTrends = (qty = 6) => {
            if (!isSocketOpen()) return; // Block
            socket.send(`${convert("GET_TRENDS")}/${0x00}/${getID(setTrends)}`); 
        }

        // Pass the current time in seconds from the ESP. If different than
        // real time, sends socket cmd to calibrate to real time.
        let calibrateTime = (seconds) => { // calibrates time if different
            const time = new Date();
            const padding = 2; // This prevents constant cal w/ rounding err.
            let secPastMid = (time.getHours() * 3600) + (time.getMinutes() * 60) 
                + time.getSeconds();

            // Compute time delta between machine and client.
            const delta = ((seconds - secPastMid)**2)**(1/2);
   
            // calibrate clock if esp time out of range. Ignore midnight switch.
            if (delta >= padding && secPastMid != 0) { 
                if (!isSocketOpen()) return; // Block
                socket.send(`${convert("CALIBRATE_TIME")}/
                ${secPastMid}/${getID(defResp)}`);
            }
        }

        let handleTempHum = () => {
           
            let checkBounds = (condition, data, eleReID, eleAltID) => {
               
                if (condition) { // Opening conditon for relay only. Ensure set.
                    if (((data[4] == 0) && (data[0] < data[5])) ||
                        ((data[4] == 1) && (data[0] > data[5]))) {

                        eleReID.classList.add("boundBust");
                    } else {
                        eleReID.classList.remove("boundBust");
                    }
                } else {
                    eleReID.classList.remove("boundBust");
                }

                if (((data[6] == 0) && (data[0] < data[7])) ||
                    ((data[6] == 1) && (data[0] > data[7]))) {

                        eleAltID.classList.add("boundBust");
                    } else {
                        eleAltID.classList.remove("boundBust");
                    }
            }

            let proc = (cont, data, name) => {
                
                const parent = document.getElementById(cont.parentID);
                const IDs = Object.keys(cont.readOnly);
                const val = document.getElementById(IDs[0]);
                const re = document.getElementById(IDs[1]);
                const alt = document.getElementById(IDs[2]);
                const U = isCelcius ? ' C' : ' F';

                // Append units onto name.
                name = (name === "Temp") ? name += U : name += ' %';

                val.innerText = `${name}: ${data[0].toFixed(1)} |` +
                    ` Avg: ${data[1].toFixed(1)} |` +
                    ` Prev Avg: ${data[2].toFixed(1)}`;
                re.innerText = `Plug: ${relayNum(data[3])} set to` +
                    ` ${reAltCond(data[4], data[5])}`;
                
                alt.innerText = `Alert set to ${reAltCond(data[6], data[7])}`;

                if (allData.SHTUp) {
                    parent.style.backgroundColor = "rgb(0, 224, 27)";
                } else {
                    parent.style.backgroundColor = "yellow";
                }

                checkBounds(data[4] != 2 && data[3] != RE_OFF, data, re, alt);
            }

            let manipTemp = (data) => { // Do not affect standing values.
                data[5] /= 100; // reduce to actual float
                data[7] /= 100; // reduce to actual float
                const prohibIdx = [3, 4, 6]; // Ignore these values in loop.

                if (!isCelcius) {
    
                    data.forEach((item, idx) => {
                        if (prohibIdx.indexOf(idx) != -1) return; // match
                        data[idx] = ((item * 1.8) + 32); // Conv to F.   
                    });
                }
            }

            let data = [allData.temp, allData.tempAvg, allData.tempAvgPrev,
                allData.tempRe, allData.tempReCond, allData.tempReVal,
                allData.tempAltCond, allData.tempAltVal];

            manipTemp(data);
            proc(tempCon, data, "Temp");

            data = [allData.hum, allData.humAvg, allData.humAvgPrev,
                allData.humRe, allData.humReCond, allData.humReVal,
                allData.humAltCond, allData.humAltVal];

            proc(humCon, data, "Hum");
            
        }

        let handleSoil = () => {
            const cont = [soil1Con, soil2Con, soil3Con, soil4Con];

            let checkBounds = (data, eleAltID) => {
                if (((data[1] == 0) && (data[0] < data[2])) ||
                    ((data[1] == 1) && (data[0] > data[2]))) {

                    eleAltID.classList.add("boundBust");
                } else {
                    eleAltID.classList.remove("boundBust");
                }
            }

            let proc = (cont, data, name) => {
                const parent = document.getElementById(cont.parentID);
                const IDs = Object.keys(cont.readOnly);
                const val = document.getElementById(IDs[0]);
                const alt = document.getElementById(IDs[1]);

                val.innerText = `${name}: ${data[0]}`;
                alt.innerText = `Alert set to ${reAltCond(data[1], data[2])}`;
   
                if (allData[`${name}Up`]) {
                    parent.style.backgroundColor = "rgb(0, 224, 27)";
                } else {
                    parent.style.backgroundColor = "yellow";
                }

                checkBounds(data, alt);
            }

            cont.forEach((sensor, idx) => {
                let tIdx = idx + 1;
                
                let data = [allData[`soil${tIdx}`], 
                    allData[`soil${tIdx}AltCond`], allData[`soil${tIdx}AltVal`]];
                proc(sensor, data, `soil${tIdx}`);
            });
        }

        let handleLight = () => {
            // photo container as well as 
        }

        // RECEIVED MESSAGE HANDLERS

        // Once all data is req in polling, calls this with reply.
        let getAll = (data) => { // Sets the addData object to response
            allData = data; // Allows use between poll interval waits
            
            const title = document.getElementById("title");
            title.innerHTML = `Greenhouse Monitor V${data.firmv}`;

            calibrateTime(data.sysTime); // Ensures sys clock is calib to client
            handleTempHum();
            handleSoil();
            handleLight();

            if (data.newLog === 1) getLog(); // Gets log if avail
        }

        // Requires the html element ID, and value to update it to. Only runs
        // upon a successful esp-32 change indicated in socket reply. Changes
        // innerHTML in the event the value is HTML.
        let defResp = (KVs = null, CBfunc = null) => { // Default response.
            if (KVs) {
                Object.keys(KVs).forEach(key => {
                    allData[key] = KVs[key];
                });
            }

            if (CBfunc) CBfunc();
        };

        // Set the trends when received back from socker server.
        let setTrends = (data) => (trends = data); // Copies data to trends.

        // All peripheral sensor data and controls.
        let build = (container) => {

            Object.keys(container.readOnly).forEach(ele => {
                const data = container.readOnly[ele];
                const e = document.createElement(data.eleType);
                e.id = ele;
                e.innerText = data.text
                data.classes.forEach(cls => { // add classes to list.
                    if (cls == null || cls == undefined) return;
                    e.classList.add(cls);
                });
                document.getElementById(container.parentID).appendChild(e);
            });
        }

        let buildLabel = (parent, data) => {
            const label = document.createElement("div");
            if (data.label === "Adjust") {
                label.style.gridColumn = "1"; // Span 1 if adj.
            } else {
                label.style.gridColumn = "1 / 3"; // Span 2 if regular inp
            }
            label.innerText = data.label; 
            label.style.alignContent = "center";
            label.id = `${parent}Lab`;
            return label;
        }

        // Input will always be displayed. If a rangeMap is sent, this will make
        // the input be a display only, and the true value will be stored hidden.
        let buildInput = (parent, data) => {
            const ele = document.createElement(data.eleType);
            ele.readOnly = true;
            ele.id = parent;

            if (!data.dataPtr) {ele.value = 0; return ele;} 

            if (data.dataPtr && data.rangeMap) {
                // Checks the special map if data outside of range map.
                if (data.specMap && !data.rangeMap[allData[data.dataPtr]]) {
                    ele.value = data.specMap[allData[data.dataPtr]];
                } else {
                    ele.value = data.rangeMap[allData[data.dataPtr]];
                }

            } else {
                ele.value = allData[data.dataPtr]
            }

            return ele;
        }

        // builds submit button for input items.
        let buildButton = (parent, data) => {
            const button = document.createElement("button");
            button.id = `${parent}But`;
            button.innerText = "Submit";
            button.onclick = () => data.butFunc();
            return button;
        }

        let buildHidden = (parent, data) => { // This will contain real values
            const actual = document.createElement(data.eleType);
            actual.type = "hidden";
            actual.id = `${parent}Hidden`;
            actual.value = allData[data.dataPtr]; 
            return actual;
        }

        let addListen = (disp, actual, ID, data) => {

            disp.addEventListener("focus", () => {
                const adj = document.getElementById(`${ID}Adj`);
                
                adj.innerHTML = `<input id="${ID}AdjBar" 
                    type="range" min="${data.min}" max="${data.max}" 
                    step="${data.step}">`;

                const AdjBar = document.getElementById(`${ID}AdjBar`);

                AdjBar.addEventListener("input", () => {
                    if (actual) (actual.value = AdjBar.value);
                    
                    if (data.rangeMap != null) {
                        disp.value = data.rangeMap[AdjBar.value];
                    } else {
                        disp.value = AdjBar.value;
                    }
                });

                if (data.rangeMap != null) { // Set upon expansion.
                    AdjBar.value = data.rangeMap.indexOf(disp.value);
                } else {
                    AdjBar.value = disp.value; 
                }

                disp.classList.add("selected");
            });

            disp.addEventListener("blur", () => {
                disp.classList.remove("selected");
            });
        }

        let expand = (container) => {
            const button = document.getElementById(container.buttonID);
            const obj = container.inputs;
            const ID = container.parentID;
            let idx = 0; // USed for expansion elements.

            let append = (ele) => {
                document.getElementById(ID).appendChild(ele);
                Expansions[ID][idx++] = ele.id;
            }

            if (button.innerText === "Expand") {
                Expansions[ID] = []; // Clear before use.
                let idx = 0; // Used for expansion elements.

                Object.keys(obj).forEach(ele => {
                    const data = obj[ele];

                    const label = buildLabel(ele, data);
                    const e = buildInput(ele, data);

                    // Build hidden carrier input if range map attached.
                    const hidden = data.rangeMap ? buildHidden(ele, data) : null;
                    const but = data.incBut ? buildButton(ele, data) : null;
                    append(label);
                    append(e);
                    if (hidden != null) append(hidden);
                    if (but != null) append(but);
          
                    data.classes.forEach(cls => { // Add classes
                        if (cls == null || cls == undefined) return; // none
                        e.classList.add(cls);
                    });

                    // If non-input, do not set event listeners.
                    if (data.eleType === "input") addListen(e, hidden, ID, data);
                });

                button.innerText = "Contract";
   
            } else if (button.innerText === "Contract") {
                Expansions[ID].forEach(id => {
                    document.getElementById(id).remove();
                });

                button.innerText = "Expand";
            }
        }

        let relayNum = (val) => val === RE_OFF ? "NONE" : Number(val) + 1;
        let reAltCond = (cond, val) => {
            const textConv = ["<", ">", "NONE"];
            if (cond == 2) return textConv[cond];
            return `${textConv[cond]} ${val.toFixed(1)}`;
        }

        // Pings the server to see if new firmware is available. If the server 
        // has a fw version different than the device, it will reply with the
        // new version, which will allow a clickable update.
        const checkNewFW = () => {
            return new Promise((resolve, reject) => {
            fetch(OTAURL)
            .then(res => res.json())
            .then(res => {
                const {version} = res;
                const noActionResp = ["Invalid JSON", "match", "wap", 
                    "Connection open fail", "Connection init fail"];
                    

                // If -1, means that there is an actual value, so the update
                // should be available in button form.
                if (noActionResp.indexOf(version) == -1) {
                    const html = `<button id="FWbut" class="sleekButton" 
                        onclick="DLfirmware('${res.signatureURL}', 
                        '${res.firmwareURL}')"> Update to Version ${version}
                        </button>`;

                    OTAdisp.innerHTML = html;
                    resolve(200);
                }
            })
            .catch(err => reject(err));
        });
        }

        // Triggered by the DL firmware button. Downloads the OTA update,
        // and if successful, restarts the webpage. This will clear the button
        // since the versions will be matched.
        const DLfirmware = (sigURL, firURL) => {
            const URL = `${URLprotocol}://${URLbody}/OTAUpdate?url=${firURL}
                &sigurl=${sigURL}`;

            let updHTML = OTAdisp.innerHTML;

            fetch(URL)
            .then(res => res.json())
            .then(res => {
                const {status} = res;
                if (status === "OK") { // Exp response from the server.
                    let secToReload = 10;
                    let intervalID = setInterval(() => { // Countdown to reload
                        OTAdisp.innerText = `Restarting in ${secToReload--}`;
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

        // When called, sets a timeout for 15 sec to check for OTA updates,
        // and start interval to update after n amount of time.
        let checkUpdates = () => {

            let runCheck = (curTime) => { // Runs 
                checkNewFW()
                .then(resp => {
                    localStorage.setItem("OTA", curTime);
                })
                .catch(err => console.log(err));
            }

            setTimeout(() => runCheck(Date.now()), 15000); // Chk after 15 sec

            setInterval(() => {
                const lastCheck = Number(localStorage.getItem("OTA"));
                const curTime = Date.now();
   
                if (lastCheck === null) { // Has not been saved yet, first init.
                    runCheck(curTime);
                } else {
                    const expireTime = lastCheck + CHK_OTA_INTV;
                    if (curTime >= expireTime) runCheck(curTime);
                }

            }, FW_CHECK_INTV);
        }

       
        const tempCon = buildCon("temp", 
            new cmdBuild("ATTACH_TEMP_RELAY", handleTempHum),
            new cmdBuild("SET_TEMP_RE_LWR_THAN", handleTempHum),
            new cmdBuild("SET_TEMP_ALT_LWR_THAN", handleTempHum));

        const humCon = buildCon("hum", 
            new cmdBuild("ATTACH_HUM_RELAY", handleTempHum),
            new cmdBuild("SET_HUM_RE_LWR_THAN", handleTempHum),
            new cmdBuild("SET_HUM_ALT_LWR_THAN", handleTempHum));

        const soil1Con = buildCon("soil1", null, null, 
            new cmdBuild("SET_SOIL1_ALT_LWR_THAN", handleSoil));

        const soil2Con = buildCon("soil2", null, null, 
            new cmdBuild("SET_SOIL2_ALT_LWR_THAN", handleSoil));

        const soil3Con = buildCon("soil3", null, null, 
            new cmdBuild("SET_SOIL3_ALT_LWR_THAN", handleSoil));

        const soil4Con = buildCon("soil4", null, null, 
            new cmdBuild("SET_SOIL4_ALT_LWR_THAN", handleSoil));

        const lightCon = buildCon("light", 
            new cmdBuild("ATTACH_LIGHT_RELAY", handleLight),
            new cmdBuild("SET_LIGHT_RE_LWR_THAN", handleLight), null);

        const buildContainers = () => {
            build(tempCon);
            build(humCon);
            build(soil1Con);
            build(soil2Con);
            build(soil3Con);
            build(soil4Con);
            build(lightCon);
        }

        const addListeners = () => {
        document.getElementById(tempCon.buttonID).onclick = () => expand(tempCon);
        document.getElementById(humCon.buttonID).onclick = () => expand(humCon);
        document.getElementById(soil1Con.buttonID).onclick = () => expand(soil1Con);
        document.getElementById(soil2Con.buttonID).onclick = () => expand(soil2Con);
        document.getElementById(soil3Con.buttonID).onclick = () => expand(soil3Con);
        document.getElementById(soil4Con.buttonID).onclick = () => expand(soil4Con);
        document.getElementById(lightCon.buttonID).onclick = () => expand(lightCon);
        }

        let loadPage = () => {
            buildContainers();
            addListeners();
            initWebSocket(); // Inits the websocket protocol.
            getLog(); // Gets the log when loading the page.
            getTrends(6); // Gets the previous 6 hours of trend data.
            checkUpdates(); // Starts firmware checking timeout and interval.
        }

    </script>
    
</body>
</html>
)rawliteral";

}

#endif // WEBPAGES_HPP