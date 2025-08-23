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

// Main page for both station and WAP modes. Has built in JS to block STA
// features while in AP mode.
const char MAINpage[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Greenhouse Controller</title>
    <style>
        
        button {
            border: 2px double silver;
            border-radius: 5px;
            min-height: 3em;
        }

        input {
            height: 2em;
        }

        body {
            color: black;
            background-color: rgb(184, 197, 255);
            font-weight: 500;

            /* prevents mobile button hold from zooming in on text 
               Make class in future if this becomes problematic, and apply it 
               to certain features.
            */
            -webkit-user-select: none;  /* Safari */
            -moz-user-select: none;     /* Firefox */
            -ms-user-select: none;      /* IE10+ */
            user-select: none;          /* Standard */
            
            -webkit-touch-callout: none; /* Disable callout (iOS Safari) */
            -webkit-tap-highlight-color: transparent; /* Remove tap highlight */
        }

        #title {
            text-align: center;
            font-size: 1.5em;
        }

        .logFWbut {
            text-align: center;
            background-color: black;
            color: white;
            border-radius: 5px;
            font-weight: 700;
            margin: 10px 0px 10px 0;
            justify-content: center;
        }

        #FWbut {background-color: red;} 

        /* 
        Columns are dictated by pixel width, display 1 to 3, 3 on the widest.
        */
        #container { 
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 10px;
            justify-content: center;
        }

        @media (min-width: 1350px) {
            #container { /* wider cols on wider screen 3x min width 450 */
                grid-template-columns: repeat(auto-fit, minmax(450px, 1fr));
            }
        }

        .con {
            display: grid;
            grid-template-columns: repeat(4, minmax(15%, 1fr));
            grid-auto-rows: min-content; /* prevent item stretching */
            gap: 10px;
        }

        .subCon {
            background-color: rgb(85, 107, 47);
            color: white;
            padding: 20px;
            text-align: left;
            border: 2px solid rgb(0, 0, 0);
            border-radius: 5px;
        }

        .conLab { /* Container label */
            color: rgb(255, 230, 0);
            font-weight: 900;
            background-color: rgb(71, 71, 71);
        }

        .subConItem {
            padding: 5px;
            border: 2px solid silver;
            border-radius: 5px;
            text-align: left;
        }

        .adjBar {white-space: nowrap;}
        #specDisp {
            display: grid;
            grid-template-columns: max-content auto; /* 2 columns */
            grid-template-rows: auto;
            gap: 5px;
            align-self: start;
            white-space: nowrap;
            background-color: rgb(0, 0, 0);
            color: rgb(255, 255, 255);
        }

        .trends {background-color: white;}

        .span2 {grid-column: span 2;}
        .span3 {grid-column: span 3;}
        .span4 {grid-column: span 4;}
        .selected {background-color: rgb(255, 166, 0);}
        
        .boundBust, .badRange, .warning {
            background-color: rgb(196, 0, 0);
            color: white;
        }

        .good, .BGup {background-color: rgb(0, 133, 0);}
        .BGdown {background-color: yellow;}
        
        .dark {background-color: black; color: yellow; border-color: yellow;}
        .light {background-color: yellow; color: black; border-color: black;}
        .blinker {background-color: red; color: white; font-weight: 900;}
        .blinkerMild {background-color: yellow; font-weight: 900;}
        .intensityBar {
            position: relative;
            height: 20px;
        }
        .marker { /* Basic design for intensity markers */
            position: absolute;
            background: linear-gradient(to right, 
                transparent 0px, transparent 15px,
                rgb(255, 0, 0) 15px, rgb(255, 0, 0) 18px,
                transparent 18px, transparent 33px);
            width: 33px;
        }
        .marker.selected {background: rgb(255, 166, 0);}
        .reSel {border: 5px solid black;}
        .jrnlText {
            font-family: monospace;
            font-size: 14px;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
            resize: none; /* Allow vertical resizing */
            width: 100%;
            min-height: 300px !important;
            box-sizing: border-box;
            background-color: rgb(187, 255, 142);
        }
    </style>
</head>
<body onload="loadPage()">
    <p id="title">Castiel's Greenhouse Loading</p>
    <div id="otaUpd"></div>
    <div id="container">
        <div id="mainCon" class="subCon"></div>
        <div id="tempCon" class="subCon"></div>
        <div id="humCon" class="subCon"></div>
        <div id="soil0Con" class="subCon"></div>
        <div id="soil1Con" class="subCon"></div>
        <div id="soil2Con" class="subCon"></div>
        <div id="soil3Con" class="subCon"></div>
        <div id="lightCon" class="subCon"></div>
        <div id="specCon" class="subCon"></div>
        <div id="re0Con" class="subCon"></div>
        <div id="re1Con" class="subCon"></div>
        <div id="re2Con" class="subCon"></div>
        <div id="re3Con" class="subCon"></div>
        <div id="jrnl" class="subCon con">
            <button id="jrnlBut" class="span4 subConItem" 
            style="text-align: center;" onclick="openJrnl()">Open Journal/Notes
            </button>
        </div>
    </div> <!-- contains all periph containers-->
    
    <button id="SR" class="logFWbut" onclick="saveReset()">Save & Reset</button>
    
    <button id="logBut" class="logFWbut" onclick="openLog()">Open Log</button>
    <div id="log"></div>

    <script>

    // TO DO: Make ADC values percentages for light and soil. Ensure that
    // trends match the percentage, vs the raw values. Maybe this can be a
    // thing for the spec as well, vs raw data.

    // Currently done with all light and soil. Working on trends. On esp now,
    // to include soil trends for each sensor. Once done, work trends to include
    // percentages vs counts, as well as appendix units such as % or F/C.

    // ATTENTION: Logic throughout contains certain name specific methods using
    // naming. So if anything is modified, check all dependencies. i.e. relay
    // checks include if (name.includes("re")), you would want to ensure that
    // any changes update that as well, for example if you renamed temp to 
    // temperature, this could cause problems.

    let MODE = 0; // 0 and 1 are AP mode, 2 is STA mode. Will be modified.
    const Devmode = false; // true allows socket access when not served by esp.

    // Network and Sockets
    const re = /(https?):\/\/([a-zA-Z0-9.-]+(:\d+)?)/; // Regex
    const URLdata = re.exec(window.location.href);
    const URLprotocol = URLdata[1]; // http or https
    const URLbody = URLdata[2]; // main url
    const OTAURL = `${URLprotocol}://${URLbody}/OTACheck`; 
    const logURL = `${URLprotocol}://${URLbody}/getLog`;
    const TITLE = ["Castiel's Greenhouse AP", "Castiel's Greenhouse STA"];
    const webSktURL = Devmode ? `ws://greenhouse.local/ws` : 
        `ws://${URLbody}/ws`;

    // Key names are colors that correspond with the JSON socket data. arr[0]
    // is the wavelength, arr[1] is the display bar color, and arr[2] is the 
    // color darkness, 0 (light) or 1 (dark), to adjust background text.
    const COLORS = {
        "violet": ["415nm", "violet", 1], "indigo": ["445nm", "indigo", 1],
        "blue": ["485nm", "blue", 1], "cyan": ["515nm", "cyan", 0],
        "green": ["555nm", "green", 1], "yellow": ["590nm", "yellow", 0],
        "orange": ["630nm", "orange", 0], "red": ["680nm", "red", 1],
        "nir": ["NIR", "rgb(140, 0, 0)", 1], "clear": ["Clear", "white", 0]
    };
    
    // Intervals (millis)
    const POLL_INTV = 1000; // Poll interval (ms) to run GET_ALL.
    const CHK_OTA_INTV = 86400000; // OTA check run.
    const CLEAR_REQ_INTV = 60000; // Clear exp skt req if non-response.
    const FW_CHECK_INTV = 12 * 60 * 60 * 1000; // Check for new firmware.

    // Flags 
    let Flags = {SKTconn:false, openLog:true};

    // Other
    const maxID = 255; // Used to reset ID num to 0 when this value is reached.
    const logDelim = ';'; // Delimiter used in logging from the server.
    const RE_OFF = 255; // Signals relay is not attached.
    let isCelcius = false;
    const sensUpBg = ["BGdown", "BGup"]; // Sensor up/down class names.
    const sensUpTxt = ["SENSOR DOWN", "SENSOR UP"];
    let specDisplay = 'C'; // 'C'urrent, 'A'verages, 'P'rev Averages.
    const DAYS = ["Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"];
    let reDays = [0, 0, 0, 0]; // Relay day tracker for socket send only.
    const IGNORE = 'P'; // Ignore or pass. Assigned to object to pass them.
    const ADC_MAX = 32767; // Analog to Digital Converter Max.
    const ADC_STEPS = 1000; // Allows from 0.0 to 100.0% stepping in the tenths.
    const SPEC_MAX = 65535; // 16-bit spectral display.

    // All commands from socketHandler.hpp. These must correspond with the
    // enum. Will be used to return the index num, which will be the cmd.
    let CMDS = [null, // Used to index to a +1
        "GET_ALL", "CALIBRATE_TIME", "NEW_LOG_RCVD", "RELAY_CTRL", 
        "RELAY_TIMER", "RELAY_TIMER_DAY", "ATTACH_RELAYS", "SET_TEMPHUM", 
        "SET_SOIL", "SET_LIGHT", "SET_SPEC_INTEGRATION_TIME", "SET_SPEC_GAIN", 
        "CLEAR_AVERAGES", "CLEAR_AVG_SET_TIME", "SAVE_AND_RESTART", 
        "GET_TRENDS"];

    // Declare all vars here, to save space. idNum is used to keep track of
    // socket commands, allData contains all sensor data, and log contains
    // all log entries.
    let socket, poll, clearReqID, requestIDs = {}, idNum = 0,
        allData = {}, log = [], Expansions = {}, Markers = {};

    // START CONTAINER BUILDING ================================================

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
    let inputBuild = function(ranges, eleType, label, incBut, 
        classes, dataPtr, rangeMap, specMap, butFunc) {

        this.ranges = ranges; // All the applicable input ranges
        this.eleType = eleType; // element type. i.e. "input".
        this.label = label; // input box label.
        this.incBut = incBut; // bool. Include button.
        this.classes = classes; // array of classes.
        this.dataPtr = dataPtr; // pointer/key of allData JSON.

        // Maps. The range map is an array that has display values with an
        // index that corresponds with an actual numberical value. Special
        // maps are used for values outside of the typical range and are 
        // passed as objects, with idx 0 being the display, and idx 1 being the
        // value.
        this.rangeMap = rangeMap; 
        this.specMap = specMap;
        this.butFunc = butFunc; // function applied to button onclick.
        return this;
    }

    // Command build. Pass the command string, and the callback function. 
    // Returns constructed object that will be called when submit is selected.
    let cmdBuild = function(cmd, callBack) {
        this.cmd = cmd;
        this.callBack = callBack;
        return this;
    }

    // Range build. Pass the min, max, and step values. This will be used for 
    // inputs and to run quality checks for submitting data.
    let rangeBuild = function(min, max, step) {
        this.min = min; // range minimum
        this.max = max; // range max
        this.step = step; // Step size
        return this;
    }

    // Primary build container. Each sub-container will be built here. Builds
    // Read Only (RO), which will be the display, and Input (INP), which allows
    // the client to change settings. All params from re to reTimer are 
    // commands that use action buttons. This is a cookie-cutter build method.
    let buildCon = (name, re, reSet, altSet, darkSet, integ, gain, 
        reTimer, ranges, includeLabel = false, includeStat = false, 
        isADC = false) => {
        
        let RO = {}, INP = {}; // Read only and Input objects.

        // Add the container class to give the grid attributes and allow input
        // label.
        const parent = document.getElementById(`${name}Con`);
        parent.classList.add("con"); // Add class upon creation.

        // Build all containers depending on the params.
        if (includeLabel) buildConLabel(parent, name);
        if (includeStat) buildConStat(parent);
        buildConSpecial(name, ranges, RO, INP,); // No commands attached.
        if (re) buildConRelay(re, name, ranges, RO, INP);
        if (reSet) buildConRelaySet(reSet, name, ranges, RO, INP);
        if (altSet) buildConAlertSet(altSet, name, ranges, RO, INP);
        if (darkSet) buildConDarkSet(darkSet, name, ranges, RO, INP);
        if (integ) buildConIntegrationSet(integ, name, ranges, RO, INP);
        if (gain) buildConGainSet(gain, name, ranges, RO, INP);
        if (reTimer) buildConRelayTimerSet(reTimer, name, ranges, RO, INP);
        parent.isADC = isADC ? true : false; // If ADC is used for values.

        // Once all selection have been built, build expand button and
        // adjustment bar.
        RO[`${name}ConExp`] = new RObuild("button", "Expand", ["span4"]);

        return new eleBuild(`${name}Con`, RO, INP); // Return built object.
    }

    // Requires the parent container, and the name. Builds a label for notes
    // that is saved in local storage only, and set upon blur.
    let buildConLabel = (parent, name) => {
        const label = document.createElement("input");
        label.className = "subConItem span4 conLab";
        const curVal = localStorage.getItem(`${name}Label`);
        label.value = (curVal != null) ? curVal : "CUSTOM LABEL/NOTES HERE";
        label.onblur = () => localStorage.setItem(`${name}Label`, label.value);
        parent.appendChild(label);
    }

    // Requires the parent container. Builds a status bar to show if the sensor
    // is up or down.
    let buildConStat = (parent) => {
        const statBar = document.createElement("div");
        statBar.id = `${parent.id}Stat`;
        statBar.className = "subConItem span4";
        statBar.style.textAlign = "center";
        statBar.innerText = "Sensor Loading";
        parent.appendChild(statBar);
    }

    // Requires container name, and the Read Only and Input objects. Builds
    // special container attributes that are name dependent as opposed to 
    // parameter dependent, meaning they do not have commands attached to them.
    let buildConSpecial = (name, ranges, RO, INP) => {
      
        if (name === "spec") { // Spectral display

            RO[`${name}Disp`] = new RObuild("div", null, // Primary
                ["subConItem", "span4"]); // Display for spec chart.

            RO[name] = new RObuild("div", "Spectral Info Display", 
                ["subConItem", "span4"]); // Primary display for info

        } else if (name.includes("re")) { // Relay displays

            RO[name] = new RObuild("div", `0`, ["subConItem", "span4"]); 

            RO[`${name}ConButtons`] = new RObuild("div", "", 
                ["subConItem", "span4", "con"]); // button display.

        } else if (name === "main") { // Main display

            // main RO is the time and time calibration. Alt is an alternate
            // that displays temp units and version. Avg displays clear average
            // time. Trends houses 3, 6, 9, and 12 hour trends.
            RO[name] = new RObuild("div", "0", ["subConItem", "span4"]);
            RO[`${name}Alt`] = new RObuild("div", "0", ["subConItem", "span4"]);
            RO[`${name}Avg`] = new RObuild("div", "0", ["subConItem", "span4"]);
            RO[`${name}Trends`] = new RObuild("div", null, 
                ["subConItem", "span4"]); 

            // Temperature units F or C. Uses hidden since range map is passed.
            INP[`${name}Units`] = new inputBuild(ranges.units, "input", "Units",
                true, ["subConItem"], null, ['F', 'C'], null, () => {
                const val = makeNum(`${name}UnitsHidden`);
                isCelcius = val ? true : false;
                handleMain(); handleTempHum();
            });

            INP[`${name}HrSet`] = new inputBuild(ranges.hour, "input",
                "Avg Clear Hours:", false, ["subConItem"], null, null, null, 
                null);

            INP[`${name}MinSet`] = new inputBuild(ranges.min, "input",
                "Avg Clear Minutes:", true, ["subConItem"], null, null, null, 
                () => {
                
                const cmd = convert("CLEAR_AVG_SET_TIME");
                const hr = makeNum(`${name}HrSet`);
                const min = makeNum(`${name}MinSet`);

                // Quality Checks.
                const QC1 = QC(hr, ranges.hour, `${name}HrSet`);
                const QC2 = QC(min, ranges.min, `${name}MinSet`);

                if (QC1 && QC2) {
                    let sendTime = (hr * 3600) + (min * 60);
                    sendTime = sendTime > 86340 ? 86340 : sendTime;

                    const ID = getID(defResp, {"avgClrTime": sendTime}, 
                        handleMain);

                    socket.send(`${cmd}/${sendTime}/${ID}`);
                }
            });

        // Exclusive to light and soil with intensity bar use and ADC.
        } else if (name.includes("soil") || name.includes("light")) { 
            RO[name] = new RObuild("div", "0", ["subConItem", "span4"]); // main

            // Used to show visual intensity bar.
            RO[`${name}ConIntensity`] = 
                new RObuild("div", "", ["subConItem", "span4"]);

        } else { // All other displays.
            RO[name] = new RObuild("div", "0", ["subConItem", "span4"]); 
        }
    }

    // Requires the object, its name, range, and both the read only and input
    // objects. Builds the relay number display, and its inputs for all 
    // all containers that require a relay attachment.
    let buildConRelay = (obj, name, ranges, RO, INP) => {
        RO[`${name}Re`] = new RObuild("div", "0", ["subConItem", "span4"]);

        INP[`${name}ReAtt`] = new inputBuild(ranges.re, "input", "Plug #", 
            true, ["subConItem"], `${name}Re`, ['0', '1', '2', '3', "NONE"], 
            {255: ["NONE", 4]}, () => {
                
            const cmd = convert(obj.cmd); 
            const reNum = makeNum(`${name}ReAttHidden`);

            // Updates the diplay using ESP32 protocol.
            const disp = (reNum == 4 ) ? RE_OFF : reNum;
            
            if (QC(reNum, ranges.re, `${name}ReAtt`)) {

                const ID = getID(defResp, {[`${name}Re`]: disp}, 
                    obj.callBack);

                // IAW ESP32 bitwise protocol, adjust value to send.
                let output = reNum & 0xF; // Set in the lower 4 bits.

                switch (name) {
                    // No action required or temp.
                    case "hum": output |= (1 << 4); break;
                    case "light": output |= (2 << 4); break;
                }

                socket.send(`${cmd}/${output}/${ID}`);
            }
        });
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Builds the relay set display, and its inputs for all containers
    // the require relays with less than greater than (LTGT) settings.
    let buildConRelaySet = (obj, name, ranges, RO, INP) => {
        INP[`${name}ReLTGT`] = new inputBuild(ranges.reCond, "input", 
            "Plug condition", false, ["subConItem"], `${name}ReCond`, 
            ["LT <", "GT >", "NONE"], null, null);

        INP[`${name}ReSetVal`] = new inputBuild(ranges.reSet, "input", 
            `Plug ${name} value`, true, ["subConItem"], `${name}ReVal`, null, 
            null, () => {
            
            const cond = makeNum(`${name}ReLTGTHidden`); // relay condition.
            const cmd = convert(obj.cmd); // Returns number.
            let val = makeNum(`${name}ReSetVal`);

            // If temperature, ensures that the value is in celcius and
            // multiplied by 100 IAW ESP requiremtns. This int value should
            // not exceed 327 degrees, which it never will.
            if (name === "temp") {  // Makes val an int with the fixed and mult.
                val = isCelcius ? val * 100 : 
                    (((val - 32) / 1.8).toFixed(2) * 100);
            }

            // Quality Checks
            const QC1 = QC(cond, ranges.reCond, `${name}ReLTGT`);
            const QC2 = QC(val, ranges.reSet, `${name}ReSetVal`, 
                name === "temp");

            if (QC1 && QC2) {

                // If light value, convert percentage input to int16_t output.
                val = (name === "light") ? ADCConv(false, val) : val;

                const ID = getID(defResp, 
                    {[`${name}ReCond`]: cond, [`${name}ReVal`]: val}, 
                    obj.callBack);

                // IAW ESP32 bitwise protocol, adjust value to send. 
                let output = 0; // init to 0.

                switch (name) { // Soil has no relay.
                    case "temp": 
                    output |= (1 << 25); // 1 for temperature
                    output |= ((cond & 0b11) << 16); // 2 bit max.
                    output |= (val & 0xFFFF); // Ensure only lower 16-bits
                    break;

                    case "hum": // No sensor action req, passes 0.
                    output |= ((cond & 0b11) << 16); // 2 bit max.
                    output |= (val & 0xFFFF); // only lower 16-bits.
                    break;

                    case "light": 
                    output |= (1 << 20); // 1 for photoresistor
                    output |= ((cond & 0b11) << 16);
                    output |= (val & 0xFFFF); // Max is 65535
                    break;
                }

                socket.send(`${cmd}/${output}/${ID}`);  
            }
        });
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Builds the alert set display, and its inputs for all containers
    // the require alerts with less than greater than (LTGT) settings.
    let buildConAlertSet = (obj, name, ranges, RO, INP) => {

        RO[`${name}Alt`] = new RObuild("div", "0", ["subConItem", "span4"]);

        INP[`${name}AltLTGT`] = new inputBuild(ranges.altCond, "input", 
            "Alert condition", false, ["subConItem"], `${name}AltCond`, 
            ["LT <", "GT >", "NONE"], null, null);

        INP[`${name}AltSetVal`] = new inputBuild(ranges.altSet, "input", 
            `Alert ${name} value`, true, ["subConItem"], `${name}AltVal`, 
            null, null, () => {

            const cond = makeNum(`${name}AltLTGTHidden`); // alert cond.
            let val = makeNum(`${name}AltSetVal`);
            const cmd = convert(obj.cmd); // returns number.

            // If temperature, ensures that the value is in celcius and
            // multiplied by 100 IAW ESP requiremtns. This int value should
            // not exceed 327 degrees, which it never will.
            if (name === "temp") { 
                val = isCelcius ? val * 100 : 
                    (((val - 32) / 1.8).toFixed(2) * 100);
            }

            const QC1 = QC(cond, ranges.altCond, `${name}AltLTGT`)
            const QC2 = QC(val, ranges.altSet, `${name}AltSetVal`, 
                name === "temp");

            if (QC1 && QC2) {

                // If soil value, convert percentage input to int16_t output.
                val = (name.includes("soil")) ? ADCConv(false, val) : val;

                const ID = getID(defResp, 
                    {[`${name}AltCond`]: cond, [`${name}AltVal`]: val}, 
                    obj.callBack);

                // IAW ESP32 bitwise protocol, adjust value to send.
                let output = 0;

                if (name === "temp") {
                    output |= (1 << 25); // 1 for temperature.
                    output |= (1 << 24); // 1 for alert.
                    output |= ((cond & 0b11) << 16); // Condition
                    output |= (val & 0xFFFF); // Lower 16 bits is value.

                } else if (name === "hum") {
                    output |= (1 << 24); // 1 for alert.
                    output |= ((cond & 0b11) << 16); // Condition
                    output |= (val & 0xFFFF); // Lower 16 is value.

                } else if (name.includes("soil")) {
                    const sensNum = Number(name[4]); // Last char, soil(1)
                    output |= ((sensNum & 0xF) << 20); // Sensor number
                    output |= ((cond & 0b11) << 16); // Condition
                    output |= (val & 0xFFFF); // Lower 16 is value.
                }

                socket.send(`${cmd}/${output}/${ID}`);  
            }
        });
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Builds the dark set display, and its inputs for the light 
    // container so that it can adjust the setting at which positive light 
    // duration begins. If set to 500, duration starts until it falls below 500.
    let buildConDarkSet = (obj, name, ranges, RO, INP) => {
        RO[`${name}Dur`] = new RObuild("div", "0", ["subConItem", "span4"]);
        RO[`${name}Dark`] = new RObuild("div", "0", ["subConItem", "span4"]);

        INP[`${name}DarkSet`] = new inputBuild(ranges.darkSet, "input",
            "Dark value", true, ["subConItem"], "darkVal", null, null,
            () => {

            const cmd = convert(obj.cmd); // returns number value.
            let val = makeNum(`${name}DarkSet`); // Get percentage first

            if (QC(val, ranges.darkSet, `${name}DarkSet`)) {
                val = ADCConv(false, val); // Adjust from % to int16_t.
                const ID = getID(defResp, {"darkVal": val}, obj.callBack);
                let output = val & 0xFFFF; // No other requirements besides val.
                socket.send(`${cmd}/${output}/${ID}`);
            }
            }
        );
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Specific to the spectral container, builds only the AGAIN and 
    // ASTEP inputs, using the container display for feedback.
    let buildConIntegrationSet = (obj, name, ranges, RO, INP) => {
        INP[`${name}Astep`] = new inputBuild(ranges.astep, "input",
            "Integration (step)", false, ["subConItem"], "astep", null,
            null, null
        );

        INP[`${name}Atime`] = new inputBuild(ranges.atime, "input",
            "Integration (time)", true, ["subConItem"], "atime", null,
            null, () => {

            const ASTEP = makeNum(`${name}Astep`);
            const ATIME = makeNum(`${name}Atime`);
            const cmd = convert(obj.cmd); // Returns number.

            // Quality checks.
            const QC1 = QC(ASTEP, ranges.astep, `${name}Astep`);
            const QC2 = QC(ATIME, ranges.atime, `${name}Atime`);

            if (QC1 && QC2) {

                const ID = getID(defResp, {"astep": ASTEP, "atime": ATIME}, 
                    obj.callBack);

                let output = ATIME & 0xFF; // Establish Atime on init.
                output |= ((ASTEP & 0xFFFF) << 8);
                socket.send(`${cmd}/${output}/${ID}`);
            }
            }
        );
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Specific to the spectral container, builds the gain input only,
    // with no display feature.
    let buildConGainSet = (obj, name, ranges, RO, INP) => {
        INP[`${name}Again`] = new inputBuild(ranges.again, "input",
            "Spectral Gain", true, ["subConItem"], "again", 
            ["0.5x", "1x", "2x", "4x", "8x", "16x", "32x", "64x", "128x",
                "256x", "512x"], null, () => {

            const AGAIN = makeNum(`${name}Again`);
            const cmd = convert(obj.cmd);

            if (QC(AGAIN, ranges.again, `${name}Again`)) {
                const ID = getID(defResp, {"again": AGAIN}, obj.callBack);
                socket.send(`${cmd}/${AGAIN}/${ID}`);
            }
            }
        );
    }

    // Requires the object, its name, ranges, and both the read only and input
    // objects. Builds the relay timer container with a status display, and 4 
    // inputs for on HH MM and off HH MM. 
    let buildConRelayTimerSet = (obj, name, ranges, RO, INP) => {
        RO[`${name}Stat`] = new RObuild("div", "0", ["subConItem", "span4"]);
        RO[`${name}Days`] = new RObuild("div", "0", ["subConItem", "span4"]);

        INP[`${name}HrSet`] = new inputBuild(ranges.reHour, "input",
            "On Time (HH MM):", false, ["subConItem"], null, null, null, 
            null);

        INP[`${name}MinSet`] = new inputBuild(ranges.reMin, "input",
            null, false, ["subConItem"], null, null, null, 
            null);

        INP[`${name}HrOff`] = new inputBuild(ranges.reHour, "input",
            "Off Time (HH MM):", false, ["subConItem"], null, null, null, 
            null);

        INP[`${name}MinOff`] = new inputBuild(ranges.reMin, "input",
            null, true, ["subConItem"], null, null, null, 
            () => {

            const cmd = convert(obj.cmd);
            const onHr = makeNum(`${name}HrSet`);
            const onMin = makeNum(`${name}MinSet`);
            const offHr = makeNum(`${name}HrOff`);
            const offMin = makeNum(`${name}MinOff`);

            // Quality checks. Due to 4, this is written like this to be clean.
            const QC1 = QC(onHr, ranges.reHour, `${name}HrSet`);
            const QC2 = QC(onMin, ranges.reMin, `${name}MinSet`);
            const QC3 = QC(offHr, ranges.reHour, `${name}HrOff`);
            const QC4 = QC(offMin, ranges.reMin, `${name}MinOff`);

            if (QC1 && QC2 && QC3 && QC4) {
                let output = 0;

                if (name.includes("re")) { // Ensures it is a relay.
                    const reNum = Number(name[2]); // Last char, relay(1).
                    const onTime = (onHr * 3600) + (onMin * 60);
                    const offTime = (offHr * 3600) + (offMin * 60);
                    let duration = (onTime < offTime) ? (offTime - onTime) :
                        (86400 - onTime + offTime);

                    duration = Math.floor(duration / 60); // sec to min integer.
                    let ID = -1; // Will change value depending on condition.

                    if (onTime === offTime) { // Signal to remove timer.
                        output |= ((99999 & 0x1FFFF) << 11); // bits 11-28
                        ID = getID(defResp, 
                            {[`re${reNum}TimerEn`]: 0, 
                                [`re${reNum}TimerOn`]: 99999,
                                [`re${reNum}TimerOff`]: 99999},
                            obj.callBack);

                    }  else {
                        output |= ((onTime & 0x1FFFF) << 11); // bits 11-28
                        ID = getID(defResp, 
                            {[`re${reNum}TimerEn`]: 1, 
                                [`re${reNum}TimerOn`]: onTime,
                                [`re${reNum}TimerOff`]: offTime},
                            obj.callBack);
                    }

                    output |= ((reNum & 0xF) << 28); // MSNibble
                    output |= (duration & 0x7FF); // bits 0 - 10
                    socket.send(`${cmd}/${output}/${ID}`); 
                }
            }
            }
        );
    }

    // After the container has been constructed using buildCon, it is passed 
    // here which will build the read only portion of each container.
    let buildRO = (container) => {

        const parent = container.parentID;

        // Iterates each read only key, creates and builds elements.
        Object.keys(container.readOnly).forEach(eleID => {
            const data = container.readOnly[eleID];
            const e = document.createElement(data.eleType);
            e.id = eleID;
            e.innerText = data.text
            data.classes.forEach(cls => { // add classes to list.
                if (cls == null || cls == undefined) return;
                e.classList.add(cls);
            });
            document.getElementById(parent).appendChild(e);
        });

        // Build the relay control buttons after RO is built.
        if (parent.includes("re")) {
            buildRelayButtons(container, parent[2]); // 3rd char.
        }

        if (parent.includes("soil") || parent.includes("light")) {
            const ID = `${parent}Intensity`;
            intensityBarMarker(ID, true);
        }
    }

    // END CONTAINER BUILDING. =================================================
    // START SPECIAL DISPLAY BUILDING. =========================================

    // Builds the spectral display in the spectral container. Includes 8 
    // wavelengths in the visible light spectrum, as well as Near Infrared, and
    // Clear. 
    let buildSpec = () => { // spectral display in specCon.
        const disp = document.getElementById("spec"); // Primary display bar
        const parent = document.getElementById("specDisp");

        const fontColor = ["black", "white"]; // Used for color bars.

        // Event. requires color and element/color bar. Changes the main display
        // information based on color selected or moused over.
        let updDisp = (color, ele) => {
            disp.innerText = 
                `${color.toUpperCase()} | ${ele.value}/${SPEC_MAX} counts | ` +
                `${(ele.value * 100 / SPEC_MAX).toFixed(2)}%`;
        }

        // Iterate each color, using the COLORS object to build a graphical 
        // display of each wavelength.
        Object.keys(COLORS).forEach(color => {
            const e = document.createElement("div"); // Main element.
            const lab = document.createElement("div"); // Label
            e.style.backgroundColor = `${COLORS[color][1]}`;
            e.style.color = fontColor[COLORS[color][2]];
            lab.innerText = COLORS[color][0];
            e.id = color;

            // Add count and color data to display for click or mouseover .
            e.onclick = () => updDisp(color, e);
            e.onmouseover = () => updDisp(color, e);
            lab.onclick = () => updDisp(color, e); // E holds the data.
            lab.onmouseover = () => updDisp(color, e);
            parent.appendChild(lab);
            parent.appendChild(e);
        });

        // Creates buttons to display the current, averages, or previous 
        // averages to spectral display. Changes the global specDisplay var,
        // upon selection. 
        const buttons = {"Current": 'C', "Avgs": 'A', "Prev Avgs": 'P'};
        Object.keys(buttons).forEach(name => {
            const b = document.createElement("button");
            b.innerText = name;
            b.classList.add("span2");
            b.id = `${buttons[name]}Spec`;
            b.addEventListener("click", () => {
                specDisplay = buttons[name]; // Change global
                handleSpec(); // Update changes after setting.
            });
            parent.appendChild(b);
        });
    }

    // Builds the trends display in the main container. Displays 4 buttons to
    // select the amount of hours, that when clicked, fetches from the server.
    let buildTrends = () => {

        // Main container and buttons
        const parent = document.getElementById("mainTrends");
        parent.style.color = "black";
        parent.classList.add("trends");
        parent.classList.add("con");
        const buttons = ["3-hr", "6-hr", "9-hr", "12-hr"];
        const header = document.createElement("div");
        header.classList.add("span4");
        header.innerHTML = "<b>Trends</b>";
        header.style.textAlign = "center";
        parent.appendChild(header);

        buttons.forEach(but => {
            const b = document.createElement("button");
            b.id = `trend${but}`;
            b.value = Number(but.split('-')[0]); // either 3, 6, 9, or 12.
            b.innerText = but;
            b.onclick = () => getTrends(b.value);
            parent.appendChild(b);
        });

        // Trend text data will be set here. This preps for trend setting.
        const display = document.createElement("div");
        display.id = "trendDisp";
        display.className = "con span4 trends";
        parent.appendChild(display);
    }

    // Requires the container and relay number. Builds the relay buttons OFF,
    // ON, FORCE OFF, and FORCE RMV in each relay container. This allows manual
    // manipulation of each relay, vice it being attached to a sensor only.
    let buildRelayButtons = (container, reNum) => {
        const Labs = ["OFF", "ON", "FORCE OFF", "FORCE RMV"]; // Labels
        const parent = document.getElementById(`${container.parentID}Buttons`);
        const key = container.parentID.slice(0, 3); // first 3 chars, Ex re1.

        // Iterate each label and build button functionality.
        Labs.forEach((but, idx) => { 
            const b = document.createElement("button");
            b.innerText = but;
            b.id = `${container.parentID}But${idx}`; // Ex re0ConBut0 = OFF
            b.value = idx; // Index assigns the expected button value.

            // Special key assignment to the button to allow buttons to be 
            // disabled else where, if force off is selected, well until force
            // remove is selected.
            b.enabled = true;

            b.onclick = () => {
                if (!b.enabled) return; // Block if button is disabled.
                b.enabled = false; // Prevents repetative clicks.
                const cmd = convert("RELAY_CTRL");
                let newQty = allData[`${key}Qty`]; // Updatable client qty.
                const bVal = Number(b.value); // Button value 0 - 3.
                let kvPair = {};

                switch (bVal) { // Keep within range of 0 to 10 for on and off.
                    case 0: newQty = newQty === 0 ? 0 : newQty - 1; break;
                    case 1: newQty = newQty === 10 ? 10 : newQty + 1; break;
                }

                const isMan = (bVal === 1) ? 1 : 0; // Is manual.

                // bVal @ 0 and 0 qty means relay de-energized manually.
                if ((bVal === 0 && newQty === 0) || (bVal != 0 && newQty > 0)) {
                    kvPair = {[`${key}`]: bVal, [`${key}Man`]: isMan, 
                        [`${key}Qty`]: newQty};
                    
                } else { // Energized by other client, removes manual hold.
                    kvPair = {[`${key}Man`]: isMan, [`${key}Qty`]: newQty};
                }

                const ID = getID(defResp, kvPair, handleRelays);

                let output = ((reNum & 0xF) << 4); // Set renum to init
                output |= (b.value & 0xF); // Set the control value.
                
                socket.send(`${cmd}/${output}/${ID}`);
            }

            parent.appendChild(b);
        });
    }

    // END SPECIAL DISPLAY BUILDING. ===========================================
    // START CONTAINER EXPANSION BUILDING. =====================================

    // Triggered by each containers expand button. Calls this function, passing
    // the container. This will build the user interactive portion.
    let expand = (container) => {
        const button = document.getElementById(container.buttonID);
        const inp = container.inputs;
        const ID = container.parentID;

        let append = (ele) => { // Appends to parent and appends to expansions.
            document.getElementById(ID).appendChild(ele);
            Expansions[ID].push(ele.id);
        }

        if (button.innerText === "Expand") {
            Expansions[ID] = []; // Clear before use.

            // Upon expand, if relay, include special day select buttons
            // appended before hh mm settings or any other input settings. This
            // exists due to special non-cookie cutter circumstances.
            if (ID.includes("re")) {
                const days = expandRelayDays(container); // day selection.
                append(days);
            }

            const isADC = document.getElementById(ID).isADC; // Uses ADC

            // Iterate each input element, building the label, input element,
            // button, and hidden input... if required.
            Object.keys(inp).forEach(ele => {
                const data = inp[ele];
                const label = expandLabel(ele, data, isADC); // Null if no label.
                const exInp = expandInput(ele, data, ID, isADC);
                const but = data.incBut ? expandButton(ele, data) : null;

                // Build hidden carrier input if range map attached. This 
                // allows separation of real values and display values.
                const hidden = data.rangeMap ? expandHidden(ele, data) : null;

                // Append all newly created elements to the parent container.
                if (label) append(label);
                append(exInp);
                if (hidden != null) append(hidden);
                if (but != null) append(but); // append last.
        
                data.classes.forEach(cls => { // Add classes
                    if (cls == null || cls == undefined) return; // none
                    exInp.classList.add(cls);
                });

                // input listeners will link the adjustment bar with the 
                // input box for dynamic value display.
                if (data.eleType === "input") {
                    expandListener(exInp, hidden, ID, data);
                }
            });

            // For input adjustment tools.
            const adj = document.createElement("div");
            adj.innerText = "Click Input Box To Adjust";
            adj.id = `${ID}Adj`;
            adj.className = "span4 subConItem con adjBar";
            append(adj);

            button.innerText = "Contract"; // acts as a toggle and display.
            button.interval = setInterval(() => { // Allows blinking
                button.classList.toggle("blinkerMild");
            }, 500); 
            
            blockAP(); // Removes STA req elements when not STA mode.

        } else if (button.innerText === "Contract") { 

            // Iterates the containers expansion IDs, and removes each one.
            Expansions[ID].forEach(id => {
                if (id === IGNORE) return; // Prevents removing non-exist ele.
                document.getElementById(id).remove(); // Remove elements.
            });

            button.innerText = "Expand";
            clearInterval(button.interval);
            button.classList.remove("blinkerMild");

            // Special rules to default the display to a set string.
            if (ID === "specCon") { 
                const integ = document.getElementById("spec");
                integ.innerText = "Spectral Info Display";
                integ.classList.remove("selected");
            }
        }
    }

    // Requires relay container. Creates a button display before inputs with
    // each day name. Sets the reDays global to the current selected days 
    // which will be used to send a socket command with the days the relay timer
    // is enabled. 
    let expandRelayDays = (container) => {
        const butDisp = document.createElement("div");
        butDisp.className = "con span4";
        const reNum = container.parentID[2]; // Relay number extraction.
        const dayVal = Number(allData[`re${reNum}Days`]); // relay days value.
        butDisp.id = `${container.parentID}Days`;

        reDays[reNum] = 0; // Clear global before setting.

        DAYS.forEach((day, idx) => {
            const but = document.createElement("button");
            but.id = `${container.parentID}${day}`;
            but.value = 1 << idx; // bitwise sets the correct value.
            but.innerText = day;

            // Ensure proper selection upon expansion. The button attribute is
            // in addition to ensure proper toggle.
            let isSel = (dayVal >> idx) & 0b1;
            if (isSel) {
                but.isSel = true;
                but.classList.add("selected");
                reDays[reNum] += Number(but.value); // Sum all selected days.

            } else {
                but.isSel = false;
                but.classList.remove("selected");
            }

            // Adjust the global relayDays upon changing values.
            but.onclick = () => {
                if (but.isSel) {
                    reDays[reNum] -= Number(but.value);
                    but.isSel = false;
                    but.classList.remove("selected");

                } else {
                    reDays[reNum] += Number(but.value);
                    but.isSel = true;
                    but.classList.add("selected");
                }
            }
            
            butDisp.appendChild(but);
        });

        // Submits the bitwise value along with the relay number.
        const submit = document.createElement("button");
        submit.id = `${container.parentID}DaySubmit`;
        submit.className = "span4";
        submit.innerText = "Submit Timer Days";
        submit.onclick = () => {
            const cmd = convert("RELAY_TIMER_DAY");
            const ID = getID(defResp, {[`re${reNum}Days`]: reDays[reNum]},
                handleRelays);

            let output = (reNum & 0xF) << 8; // Init with relay number.
            output |= (reDays[reNum] & 0x7F); // 7 bits only, 1 for each day.
 
            socket.send(`${cmd}/${output}/${ID}`);
        }

        butDisp.appendChild(submit);

        return butDisp;
    }

    // Builds the input labels for each input requiring the input ID, and its
    // associated data.
    let expandLabel = (inpID, data, isADC = false) => {
        if (!data.label) return null; // Prevents label if not set.

        const label = document.createElement("div");
        label.style.gridColumn = "1 / 3"; // Span 2 if regular inp

        // appends % if ADC is used to show percentage vs raw value.
        label.innerText = (isADC && data.label.includes("value")) ? 
            `${data.label} %` : data.label;

        label.style.alignContent = "center"; // Keep inline with input boxes.
        label.id = `${inpID}Lab`;
        return label;
    }

    // Requires the input ID, its corresponding data, and the container ID. 
    // Input will always be displayed. If a range map is sent, this will turn 
    // the input into display only, and a hidden input will be built and 
    // populated with a numerical value. If no map is sent, serves as both an
    // input and display.
    let expandInput = (inpID, data, contID, isADC = false) => {
        const ele = document.createElement(data.eleType);
        ele.id = inpID; 

        // If ADC is being used, display percentage values vs raw data.
        let val = (isADC && data.label.includes("value")) ? 
            ADCConv(true, allData[data.dataPtr]) : allData[data.dataPtr];

        // No pointer/key to the JSON key/val data. Handles special cases 
        // with no data pointer. This is a special manipulator that changes val.
        if (!data.dataPtr) { 

            if (inpID.includes("re")) { // Check if it is a relay.
               
                const reTag = inpID.slice(0, 3); // Will extract reN.
            
                if (allData[`${reTag}TimerEn`]) { // check for enabled
                    const rest = inpID.slice(3, inpID.length); // Remaining str

                    // Gets times in seconds
                    const onTime = allData[`${reTag}TimerOn`];
                    const offTime = allData[`${reTag}TimerOff`];

                    switch (rest) { // Sets the value to the appropriate value.
                        case "HrSet": val = timeStr(onTime, true)[0]; break;
                        case "MinSet": val = timeStr(onTime, true)[1]; break;
                        case "HrOff": val = timeStr(offTime, true)[0]; break;
                        case "MinOff": val = timeStr(offTime, true)[1]; break;
                    }

                } else {
                    val = 0; // zero out if not enabled.
                } 

            } else if (inpID === "mainUnits") {
                val = Number(isCelcius);

            } else if (inpID === "mainHrSet") {
                val = timeStr(allData["avgClrTime"], true)[0]; // Hr

            } else if (inpID === "mainMinSet") {
                val = timeStr(allData["avgClrTime"], true)[1]; // Min
            }
        }

        if (data.rangeMap) { // Checks for rangeMap. If true...
            ele.readOnly = true; // Forces use of adjustment bar.

            // Checks for a special map and that the range map value does not
            // eixst. If true, sets to special map, if false, uses range map/
            if (data.specMap && !data.rangeMap[val]) {
                ele.value = data.specMap[val][0];
            } else {
                ele.value = data.rangeMap[val];
            }

        } else {

            ele.readOnly = false; // Allows use of input or adj bar.

            // ESP rules dictate that temp is passed in C multiplied by 100 for
            // float point precision. 
            if (contID === "tempCon") {
                ele.value = isCelcius ? Number(val / 100).toFixed(1) :
                    Number(((val / 100) * 1.8) + 32).toFixed(1);
            } else {
                ele.value = val;
            }
        }

        return ele;
    }

    // Stores the actual numerical values if a range map is passed along with
    // an input.
    let expandHidden = (inpID, data) => { 
        const actual = document.createElement(data.eleType);
        actual.type = "hidden";
        actual.id = `${inpID}Hidden`;

        let val = Number(allData[data.dataPtr]); // Set upon init.

        // Check if part of the special map first. Iterate each key, and if
        // matched, updates the value with req value. Special maps are used 
        // only for data outside the range scope, no range map required here.
        if (data.specMap) {
            Object.keys(data.specMap).forEach(key => {
                if (key == val) val = data.specMap[allData[data.dataPtr]][1];
            });
        }

        // Manipulate based on special data.
        if (inpID === "mainUnits") val = Number(isCelcius);

        actual.value = val; // Set the value after checks complete.
        return actual;
    }

    // builds submit button for selected input items. Requires input ID and its 
    // corresponding data. 
    let expandButton = (inpID, data) => {
        const button = document.createElement("button");
        const isRelay = inpID.includes("re");
        const reText = "Submit Time (hold to disable timer)"; // if isRelay
        button.id = `${inpID}But`;
        button.holdTimeout = null; // Store within button.
        
        // Assigns a timeout to holdTimeout. If hold complete for 1 sec,
        // zeros all values and submits, which will trigger a disable timer.
        let startHold = () => { 
            button.holdTimeout = setTimeout(() => {
                const tag = inpID.slice(0, 3); // Ex re1, re2, etc...
                document.getElementById(`${tag}HrSet`).value = 0;
                document.getElementById(`${tag}MinSet`).value = 0;
                document.getElementById(`${tag}HrOff`).value = 0;
                document.getElementById(`${tag}MinOff`).value = 0;
                button.innerText = "RELEASE"; // Prompts release to submit.
                button.classList.add("warning"); // changes color.
            }, 1000);
        }
        
        let endHold = () => { // Releases timeout.
            clearTimeout(button.holdTimeout);
            button.innerText = reText;
            button.classList.remove("warning");
            data.butFunc();
        }

        button.innerText = isRelay ? reText : "Submit";
        
        // Special conditions for relay buttons. Prevents click and mouse events
        // from contradicting.
        if (inpID.includes("re")) {
            button.classList.add("span4");
            button.addEventListener("pointerdown", startHold);
            button.addEventListener("pointerup", endHold);

        } else {
            button.onclick = () => data.butFunc(); // full check.
        }

        return button;
    }

    // Event listener exclusive to the all inputs in the container. Requires 
    // input, the hidden input, the parent container ID, and the associated
    // input data. Adds event listener to inputs to allow adjustment bar use.
    let expandListener = (input, hidden, parent, data) => {
    
        // The adjusted value is passed and the display value will be adjusted
        // as well as the hidden (actual) value if a range map is passed.
        let adjVals = (adjVal) => { 

            if (hidden) (hidden.value = adjVal); // Actual numerical value.

            // Adjust the display value using the range map or the adj value.
            input.value = (data.rangeMap === null) ? 
                adjVal : data.rangeMap[adjVal];

            // Allow display of integration time when adjusting values.
            if (parent === "specCon") { 
                const ASTEP = makeNum("specAstep");
                const ATIME = makeNum("specAtime");
                const AGAIN = makeNum("specAgain");
                const DISP = document.getElementById("spec"); // main element

                // Integration time is computed as per the AS7341 datasheet.
                const intTime = (ASTEP + 1) * (ATIME + 1) * 0.00278;

                // display the integration time in the display window.
                DISP.innerText = `Integration Time: ${intTime.toFixed(1)} ms`;
                DISP.classList.add("selected");
            }
        }
        
        // When an input is focused, It takes ownership of the adjustment bar.
        input.addEventListener("focus", () => {

            let {min, max, step} = data.ranges; // Set vars.
            
            // If F is set instead of C, adjusts the min, max, and step.
            if (["tempReVal", "tempAltVal"].includes(data.dataPtr) &&
                !isCelcius) { 
                min = (min * 1.8) + 32;
                max = (max * 1.8) + 32;
                step = 1; // smaller step size only for celcius.
            }

            // On focus, adds an adjustment bar.
            const adj = document.getElementById(`${parent}Adj`);
            adj.innerHTML = `<input id="${parent}AdjBar" type="range" ` + 
                `class="span4" min="${min}" max="${max}" ` +
                `step="${step}">`;
            
            const AdjBar = document.getElementById(`${parent}AdjBar`);
            AdjBar.addEventListener("input", () => adjVals(AdjBar.value));

            // Specify params for buttons. These buttons increase the 
            // adjustment granularity to assist the adjustment bar.
            ['-', '+'].forEach(label => { 
                const but = document.createElement("button");
                but.classList.add("span2");
                but.id = `${parent}${label}`; // Should be like tempCon+
                but.innerHTML = `<b>${label} ${step}</b>`;
                adj.appendChild(but);

                // Event listener directly controls the adjbar, which then 
                // Adjusts the box value.
                but.addEventListener("click", () => {
                    if (label === '+') {
                        AdjBar.value = Number(AdjBar.value) + Number(step);
                    } else {
                        AdjBar.value = Number(AdjBar.value) - Number(step);
                    }
                    
                    adjVals(AdjBar.value);
                });
            });

            // Set the adjBar value upon linking it to an input. If range map
            // is included, defaults to the hidden value, if not, uses input.
            AdjBar.value = (data.rangeMap) ? hidden.value : input.value;

            // Specmap omitted, not required. Specmap used only when converting
            // ESP JSON to cliently friendly format.

            input.classList.add("selected");
        });

        input.addEventListener("blur", () => {
            input.classList.remove("selected");
        });
    }

    // END CONTAINER EXPANSION BUILDING. =======================================
    // START CONTAINER SUPPLEMENTARY FUNCTIONS. ================================

    let makeNum = (ID) => { // Used to ref ID, and return Number of the value.
        return Number(document.getElementById(ID).value);
    }

    // Requires the value, its ranges, the input ID, and if it is a temperature
    // reading. Upon success, changes input BG to match, and same with failure,
    // which also prompts an alert. 
    let QC = (val, ranges, inpID, isTemp = false) => {
        const ele = document.getElementById(inpID);

        // If temperature, div by 100 IAW ESP temperature protocol. This is 
        // because temperature (VAL ONLY) will by mult by 100 before sending.
        val = isTemp ? val / 100 : val; 

        if (val >= ranges.min && val <= ranges.max) { // Check within range.
            ele.classList.remove("badRange")
            ele.classList.add("good");
            return true;

        } else { // Not within range.
            ele.classList.add("badRange");
            ele.classList.remove("good");

            if (isTemp && !isCelcius) { // Applies to faren value only.
                window.alert(`${((val * 1.8) + 32).toFixed(1)} out of range ` +
                    `${((ranges.min * 1.8) + 32).toFixed(1)} to ` +
                    `${((ranges.max * 1.8) + 32).toFixed(1)}`);

            } else { // All other alerts.
                window.alert(`${val} out of range ${ranges.min} to ` +
                    `${ranges.max}`);
            }

            return false;
        }
    }

    // Timestring requires seconds after midnight, and returnRaw which is def
    // to false. Gives a string if false, and if true, returns array with 
    // [hh, mm, ss]. Ensures 2 digits per value and all vals are strings.
    let timeStr = (seconds, returnRaw = false) => {
        const h = Math.floor(seconds / 3600).toString().padStart(2, '0');
        const m = Math.floor((seconds % 3600) / 60).toString().padStart(2, '0');
        const s = (seconds % 60).toString().padStart(2, '0');
        return (returnRaw) ? [h, m, s] : `${h}:${m}:${s}`;
    }

    // END CONTAINER SUPPLEMENTARY FUNCTIONS. ==================================
    // START SOCKET FUNCTIONS. =================================================

    // Returns the index of the command, this is used with esp32 socket
    // command as argument 0.
    const convert = (CMD) => Number(CMDS.indexOf(CMD));

    // Requires callback function for a successful update, the key/value
    // pairs with the key being the allData JSON key, and the value to
    // update to, and the second callback function which will run after
    // the JSON has been updated, to immediately update display if occured 
    // between polling all data. Returns ID which will be used for handling.
    const getID = (callback1 = null, KVs = null, callback2 = null) => {
        const id = idNum++; // between 0 and 255, rotating.
        requestIDs[id] = [callback1, KVs, callback2]; 
        idNum = (idNum >= maxID) ? 0 : idNum; // Reset to 0 if == max.
        return id;
    }

    // No params. Returns true if socket is open, false if not.
    const isSocketOpen = () => (Flags.SKTconn && WebSocket.OPEN);

    // SOCKETS. All event handlers init here.
    const initWebSocket = () => {
        console.log("Init websocket");
        socket = new WebSocket(webSktURL); // Open new socket.
        socket.onopen = socketOpen; // Set the event handler functions.
        socket.onclose = socketClose;
        socket.onmessage = socketMsg;
    }

    const socketOpen = () => { // Open socket handler and start polling.
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
        setTimeout(initWebSocket, 2000); // Attempt reconnect after 2 sec.
    }

    // Messages from server will be in JSON format, parsed, and sent to the
    // assigned function to handle that response.
    const socketMsg = (event) => {handleResponse(JSON.parse(event.data));}

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

    // END SOCKET FUNCTIONS. ===================================================
    // START SOCKET INCOMING RESPONSE FUNCTIONS. ===============================
    
    // Requires parsed JSON response from skt. Extracts callback function and
    // runs if exists. Deletes the request ID upon receiving response. 
    const handleResponse = (response) => {
        response.id = Number(response.id); // Explicity convert to number.
        const func = requestIDs[response.id][0] ? // Set to null if non-exist.
            requestIDs[response.id][0] : null;
    
        // Check if default response. If so, will send the html element
        // ID and the value to set it to iff successful. This will handle
        // most socket responses.
        if (func === defResp && response.status === 1) { // 1 = success
            func(requestIDs[response.id][1], requestIDs[response.id][2]);
        } else if (func != null) { // no resp status returned for non-defResp.
            func(response); // Used moreso for trends and get all.
        } // else do nothing as nothing changes.

        delete requestIDs[response.id]; // Delete corresponding ID.
    }

    // This is called after polling, and responds to all rcvd data.
    let getAll = (data) => { 
        allData = data; // Allows use between poll interval waits by copying.
        MODE = data.netMode; // Update the mode to handle particular functions.
        document.getElementById("title").innerText = TITLE[MODE === 2 ? 1 : 0];
        
        calibrateTime(data.sysTime); // Ensures sys clock is calib to client
        blockAP(); // Remove any elements that must be in station mode.

        // Run all container/peripheral handlers to update if req.
        handleMain(); handleTempHum(); handleSoil(); handlePhoto();
        handleSpec(); handleRelays();

        if (data.newLog === 1) getLog(); // Gets log if avail.
    }

    // Requires the html element ID, and value to update it to. Only runs
    // upon a successful esp-32 change indicated in socket reply. Changes
    // innerHTML in the event the value is HTML.
    let defResp = (KVs = null, CBfunc = null) => { // Default response.
        if (KVs) Object.keys(KVs).forEach(key => allData[key] = KVs[key]);
        if (CBfunc) CBfunc(); 
    };

    // Pass the current time in seconds from the ESP. If different than
    // real time, sends socket cmd to calibrate to real time.
    let calibrateTime = (seconds) => { // calibrates time if different
        const time = new Date();
        const padding = 2; // This prevents constant cal w/ rounding err.
        let secPastMid = (time.getHours() * 3600) + (time.getMinutes() * 60) 
            + time.getSeconds(); // init with seconds past midnight.

        // Compute time delta between machine and client.
        const delta = ((seconds - secPastMid)**2)**(1/2);

        // calibrate clock if esp time out of range. Padding exists to ensure
        // rounding errors do not prompt a re-calibration.
        if (delta >= padding && secPastMid != 0) { 
            if (!isSocketOpen()) return; // Block
            let day = time.getDay();
            day = (day === 0) ? 6 : day - 1; // Ensure day 0 = monday.

            let output = (secPastMid & 0x1FFFF); // init with sec past midnight.
            output |= ((day & 0b111) << 17);
            socket.send(`${convert("CALIBRATE_TIME")}/${output}/` +
                `${getID(defResp)}`);
        }
    }

    // Main container. Updates the RO portion after main poll.
    let handleMain = () => {

        const units = ['F', 'C']; // Used for unit display.
        const isCal = ['N', 'Y']; // Used for is time calibrated display.

        document.getElementById("main").innerText = 
            `Time: ${DAYS[allData["day"]]} @ ${timeStr(allData["sysTime"])} |` +
            ` Calibrated: ${isCal[allData["timeCalib"]]}`;

        document.getElementById("mainAlt").innerText = `Temp Units: ` +
            `${units[Number(isCelcius)]} | Version: ${allData["firmv"]}`;

        document.getElementById("mainAvg").innerText = 
            `Clear Avgs @ ${timeStr(allData["avgClrTime"])}`;
    }

    // Temperature and humidity containers. Updates the RO portion by
    // iterating each individual container. Used temp and hum together since
    // they are nearly identical. 
    let handleTempHum = () => { 

        // Checks the bounds, and if the temp/hum is out of bounds, adds class.
        let checkBounds = (data, eleReID, eleAltID) => {
            const [val, avg, avgPrev, re, reCon, reVal, altCon, altVal] = data;

            // Check that relay is in play and its has a value.
            if (re != RE_OFF) { 
                // If true, check that the relay has a setting other than NONE.
                if (((reCon == 0) && (val < reVal)) ||
                    ((reCon == 1) && (val > reVal))) {

                    eleReID.classList.add("boundBust");
                } else {
                    eleReID.classList.remove("boundBust"); // ensure removal.
                }

            } else { // Relay off, bounds are out of play.
                eleReID.classList.remove("boundBust");
            }

            // Next check that the alert has a setting other than NONE.
            if (((altCon == 0) && (val < altVal)) ||
                ((altCon == 1) && (val > altVal))) {

                    eleAltID.classList.add("boundBust");
                } else {
                    eleAltID.classList.remove("boundBust");
                }
        }

        // Processes data from the allData object, to display the current
        // settings and values in the RO portion of the tempcon.
        let proc = (cont, data, name) => {
            const [val, avg, avgPrev, re, reCon, reVal, altCon, altVal] = data;

            const parent = document.getElementById(cont.parentID);
            const stat = document.getElementById(`${cont.parentID}Stat`);
            const IDs = Object.keys(cont.readOnly);
            const valRO = document.getElementById(IDs[0]);
            const reRO = document.getElementById(IDs[1]);
            const altRO = document.getElementById(IDs[2]);
            const U = isCelcius ? ' C' : ' F'; // Unit.

            // Append units and signs onto name.
            name = (name === "Temp") ? name += U : name += ' %';

            // Set the text for the RO portions of the tempcon.
            valRO.innerText = `${name}: ${val.toFixed(1)} |` +
                ` Avg: ${avg.toFixed(1)} | Prev Avg: ${avgPrev.toFixed(1)}`;

            reRO.innerText = `Plug: ${relayNum(re)} set to` +
                ` ${reAltCond(reCon, reVal)}`;

            altRO.innerText = MODE === 2 ? 
                `Alert set to ${reAltCond(altCon, altVal)}` : 
                "Alert Disabled in AP Mode";

            sensUpBg.forEach(cls => stat.classList.remove(cls)); // rmv all
            stat.classList.add(sensUpBg[allData["SHTUp"]]);
            stat.innerText = sensUpTxt[allData["SHTUp"]];
            
            // Check bounds once processed to ensure correct coloring.
            checkBounds(data, reRO, altRO);
        }

        // Changes the temperature the the appropriate value per ESP32 proto.
        let manipTemp = (data) => { // Do not affect standing values.
            // To manip, cannot use copies, must change actual index vals.
            data[5] /= 100; data[7] /= 100; // Reduce to float val.

            const prohibIdx = [3, 4, 6]; // Ignore these indicies in loop.

            if (!isCelcius) { // If units = F, convert to F.
                data.forEach((item, idx) => {
                    if (prohibIdx.indexOf(idx) != -1) return; // match, pass.
                    data[idx] = ((item * 1.8) + 32); // Conv to F.   
                });
            }
        }

        // array with all the temperature data to process.
        let data = [allData.temp, allData.tempAvg, allData.tempAvgPrev,
            allData.tempRe, allData.tempReCond, allData.tempReVal,
            allData.tempAltCond, allData.tempAltVal];

        manipTemp(data); // Manipulate the temperature before processing.
        proc(tempCon, data, "Temp");

        // update array with the humidity data to process next.
        data = [allData.hum, allData.humAvg, allData.humAvgPrev,
            allData.humRe, allData.humReCond, allData.humReVal,
            allData.humAltCond, allData.humAltVal];

        proc(humCon, data, "Hum"); 
    }

    let handleSoil = () => { // Handles all soil containers updating RO.
        const cont = [soil1Con, soil2Con, soil3Con, soil4Con]; // estab cont.

        // Changes RO display color if value exceeds bounds.
        let checkBounds = (data, eleAltID) => {
            const [val, altCon, altVal] = data;

            if (((altCon == 0) && (val < altVal)) ||
                ((altCon == 1) && (val > altVal))) {

                eleAltID.classList.add("boundBust");
            } else {
                eleAltID.classList.remove("boundBust");
            }
        }

        // processes data from the allData object, to diplay the current setting
        // and values in the RO portion of each soilCon.
        let proc = (cont, data, name) => {
            const [val, altCon, altVal] = data;

            const parent = document.getElementById(cont.parentID);
            const stat = document.getElementById(`${cont.parentID}Stat`);
            const IDs = Object.keys(cont.readOnly);
            const valRO = document.getElementById(IDs[0]); // main
            const altRO = document.getElementById(IDs[2]); // alert display

            intensityBar(IDs[1], val, ADC_MAX, "blue", "black");
            valRO.innerText = `${name.slice(0, 4)} #${name[4]}: ` +
                `${ADCConv(true, val)}%`;

            altRO.innerText = MODE === 2 ? 
                `Alert set to ${reAltCond(altCon, altVal)}` : 
                "Alert Disabled in AP Mode";

            sensUpBg.forEach(cls => stat.classList.remove(cls)); // rmv all
            stat.classList.add(sensUpBg[allData["SHTUp"]]);
            stat.innerText = sensUpTxt[allData["SHTUp"]];

            checkBounds(data, altRO);
        }

        cont.forEach((sensor, idx) => { // Iterate and process each soil con.
            const data = [allData[`soil${idx}`], allData[`soil${idx}AltCond`], 
                allData[`soil${idx}AltVal`]];

            proc(sensor, data, `soil${idx}`);
        });
    }

    // Handles the photo/light container using the photoresistor. 
    let handlePhoto = () => {
        let checkBounds = (data, eleReID, darkID, durID) => {
            const [val, avg, avgPrev, re, reCon, reVal, dur, dark] = data;

            // Check the relay is in play nad has a value.
            if (re != RE_OFF) {
                if (((reCon == 0) && (val < reVal)) || 
                    ((reCon == 1) && (val > reVal))) {

                    eleReID.classList.add("boundBust");
                } else {
                    eleReID.classList.remove("boundBust");
                }
            }

            const isDark = val < dark; // Used to change display BG color.
            darkID.classList.toggle("dark", isDark);
            darkID.classList.toggle("light", !isDark);
            durID.classList.toggle("dark", isDark);
            durID.classList.toggle("light", !isDark);
        } 

        let proc = (cont, data, name) => { // Process all container data.
            const [val, avg, avgPrev, re, reCon, reVal, dur, dark] = data;

            const parent = document.getElementById(cont.parentID);
            const stat = document.getElementById(`${cont.parentID}Stat`);
            const IDs = Object.keys(cont.readOnly);
            const valRO = document.getElementById(IDs[0]); // main
            const reRO = document.getElementById(IDs[2]); // relay info
            const durRO = document.getElementById(IDs[3]); // duraiton info
            const darkRO = document.getElementById(IDs[4]) // dark info
            const barWidth = intensityBar.offsetWidth; // inten bar width
            const pxStop = Number(data[0]) / ADC_MAX * Number(barWidth);

            intensityBar(IDs[1], val, ADC_MAX, "yellow", "black");
           
            valRO.innerText = `${name}%: ${ADCConv(true, val)} | ` + 
                `Avg: ${ADCConv(true, Math.round(avg))} | ` +
                `Prev Avg: ${ADCConv(true, Math.round(avgPrev))}`;

            reRO.innerText = `Plug: ${relayNum(re)} set to ` +
                `${reAltCond(reCon, reVal, true)}`;

            durRO.innerText = `Light duration: ${timeStr(dur)}`;
            darkRO.innerText = `Dark val set to ${ADCConv(true, dark)}%`;

            sensUpBg.forEach(cls => stat.classList.remove(cls)); // rmv all
            stat.classList.add(sensUpBg[allData["SHTUp"]]);
            stat.innerText = sensUpTxt[allData["SHTUp"]];
            checkBounds(data, reRO, darkRO, durRO);
        }

        const data = [allData.photo, allData.photoAvg, allData.photoAvgPrev,
            allData.lightRe, allData.lightReCond, allData.lightReVal,
            allData.lightDur, allData.darkVal];

        proc(lightCon, data, "Light"); 
    }

    let handleSpec = () => { // Handles all spectrometry container activity.
        const specButtonIDs = ["CSpec", "ASpec", "PSpec"];
        const specMap = {'C': "", 'A': "Avg", 'P': "AvgPrev"}; // For JSON key
        const parent = document.getElementById("specCon");
        const stat = document.getElementById(`specConStat`); // up/down status.

        // display bar width. Takes the size of the spec display and subtracts
        // 80 px from it, which are reserved in css for the label, and padding.
        const barWidth = document.getElementById("specDisp").offsetWidth - 90;

        specButtonIDs.forEach(ID => { // Iterate and apply class to selected but
            const e = document.getElementById(ID);

            if (ID[0] === specDisplay) { // first char, Global variable
                e.classList.add("selected");
            } else {
                e.classList.remove("selected");
            }
        });

        let counts = {}; // Used to gather all counts for comparison.
        let maxCt = 1; // Default to 1, no reason behind this value.
        let maxClr = null; // Max count color.

        // Iterate to append counts with each color count. Depending on the
        // selected current, avg, or prev avg, the specMap is used to append 
        // the color with the right json key, with the counts being the value.
        Object.keys(COLORS).forEach(color => {
            const ctVal = allData[`${color}${specMap[specDisplay]}`];
            counts[color] = ctVal;

            // Set max count, and ignore clear, since its value will skew data.
            // Set color to the one with the highest value.
            maxClr = ((ctVal > maxCt) && (color != "clear")) ? color : maxClr;
            maxCt = ((ctVal > maxCt) && (color != "clear")) ? ctVal : maxCt;
            
        });

        const ctPerPix = maxCt / barWidth; // Counts per pixel.

        // Iterates the counts and assigns a pixed value to each. The highest
        // val will be at ~250 px, and the rest will be relative to it.
        Object.keys(counts).forEach(color => {
            const bar = document.getElementById(color);
            bar.value = counts[color];
            let width = Math.floor(counts[color] / ctPerPix); // Initial width

            // Set width not to exceed max.
            width = width > barWidth ? barWidth : width; 

            // Displays the count value in the largest bar as a reference for
            // the others.
            if (color === maxClr || color === "clear") { // d
                document.getElementById(color).innerText = 
                    `${(counts[color] / 655.35).toFixed(2)}%`;
            } else { // Remove text from rest.
                document.getElementById(color).innerText = "";
            }

            bar.style.width = `${width}px`;
        });

        sensUpBg.forEach(cls => stat.classList.remove(cls)); // rmv all
        stat.classList.add(sensUpBg[allData["SHTUp"]]);
        stat.innerText = sensUpTxt[allData["SHTUp"]];
    }

    const handleRelays = () => { // Handles all relay containers and activity.
        const cont = [re1Con, re2Con, re3Con, re4Con]; // containers
        
        // Ensure the same naming convention as buildRelayButtons().
        cont.forEach((re, idx) => {

            // Button stuff. Used to handle the force off function primarily.
            const reVal = Number(allData[`re${idx}`]); // relay value.
            const dayVal = Number(allData[`re${idx}Days`]); // day value
            const isMan = Number(allData[`re${idx}Man`]); // is manually on.
            const cliQty = Number(allData[`re${idx}Qty`]); // clients using.
            const off = document.getElementById(`re${idx}ConBut0`);
            const on = document.getElementById(`re${idx}ConBut1`);
            const forceOff = document.getElementById(`re${idx}ConBut2`);
            const forceRem = document.getElementById(`re${idx}ConBut3`);
            const days = document.getElementById(`re${idx}Days`);
            
            // Primary element. Update with clients using relay.
            const eleRO = document.getElementById(`re${idx}`);
            eleRO.innerText = `Plug #${idx} Clients: ${cliQty}`;

            // Set the RO display to show days timer is set.
            let dayStr = "Days Active: "; let dayLen = dayStr.length;

            for (let i = 0; i < 7; i++) { // Bitwise settings to extract day.
                const isSet = (dayVal >> i) & 0b1;
                if (isSet) dayStr += `${DAYS[i]} | `;
            }

            if (dayVal === 0) dayStr += "None___"; // Trailing ___ will be del.

            if (dayStr.length > dayLen) { // Delete trail delim iff concat.
                dayStr = dayStr.slice(0, dayStr.length - 3); 
            }

            days.innerText = dayStr;

            let handleSel = (activeSel) => { // Change class only for sel but.
                [off, on, forceOff, forceRem].forEach(but => {
                    if (but === activeSel) {
                        but.classList.add("selected");
                    } else {
                        but.classList.remove("selected");
                    }
                });
            }

            switch (reVal) { // Adjusts the classes based on button selection.
                case 0: // Relay is off, enable all but off.
                off.enabled = false; // Prevent clicking off if relay is off.
                on.enabled = forceOff.enabled = forceRem.enabled = true;
                handleSel(off); 
                break; 

                case 1: // Relay is on, enable all but on.
                on.enabled = isMan ? false : true; // if manual, disable.
                off.enabled = isMan ? true : false; // if manual, enable.
                forceOff.enabled = forceRem.enabled = true; 
                handleSel(isMan ? on : off); 
                break; 
     
                case 2: // Force off, disable all except force remove.
                forceRem.enabled = true; // On enable force removed button.
                off.enabled = on.enabled = forceOff.enabled = false;
                handleSel(forceOff);
         
                if (!forceRem.interval) { // Flashes, indicating must click
                    forceRem.interval = setInterval(() => {
                        forceRem.classList.toggle("blinker");
                    }, 500);
                }
                break; 

                case 3: // Force off is removed, re-enable all others.
                forceOff.enabled = false; // Ensure disabled to prev dbl click.
                off.enabled = on.enabled = forceOff.enabled = true;
                handleSel(forceRem);

                if (forceRem.interval) { // If interval, remove it.
                    clearInterval(forceRem.interval);
                    forceRem.classList.remove("blinker"); // Remove if set.
                }
                break;
            }

            // Iterate each button, if the relay value is the button value,
            // add class. Remove class if false. Class applies thick border.
            for (let i = 0; i < 4; i++) { 
                const but = document.getElementById(`re${idx}ConBut${i}`);

                if (reVal === i) {
                    but.classList.add("reSel");
                } else {
                    but.classList.remove("reSel");
                }
            }

            const IDs = Object.keys(re.readOnly);
            const RO = document.getElementById(IDs[2]); // 3rd RO box.
            const reTmrEn = allData[`re${idx}TimerEn`]; // relay timer enabled?
            const reTmrOn = allData[`re${idx}TimerOn`]; // relay timer on time.
            const reTmrOff = allData[`re${idx}TimerOff`]; // relay timer off tm.

            if (reTmrEn) { // Is timer enabled.
                RO.innerText = `Timer set from ${timeStr(reTmrOn)} to ` +
                    `${timeStr(reTmrOff)}`;

            } else { // Not enabled.
                RO.innerText = `Timer Disabled`;
            }
        });
    }

    // END SOCKET INCOMING RESPONSE FUNCTIONS. =================================
    // START GENERAL FUNCTIONS. ================================================

    // Blocks the Access point features if in AP mode. Used dynamically, if an
    // expansion has STA mode only features, and is switched to AP, they will 
    // erase.
    let blockAP = () => {
        if (MODE === 2) return; // Only runs if not in station mode.
        Object.keys(Expansions).forEach(key => {

            Expansions[key].forEach((ele, idx) => {
                if (ele.includes("Alt")) {
                    document.getElementById(ele).remove(); // rmv element
                    Expansions[key][idx] = IGNORE; // Signal to ignore.
                }
            });
        });
    }

    // Gets the log, separates it into a non-delimited array, to be used
    // for diplay at the page bottom, which is opened by openLog.
    let getLog = () => { // HTTP call
        const button = document.getElementById("logBut");
        fetch(logURL)
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error status: ${response.status}`);
            }
            return response.text(); // If no err, proceed.
        })
        .then(text => { // Now in text form.
            if (text.length <= 0 && !Devmode) throw new Error("No Log Data"); 

            // Split by delim into array and reverse for current first.
            log = text.split(logDelim).reverse(); 
            button.classList.add("good"); // Shows new log.
        
            // Is a receipt only, Allows server to remove flag.
            if (!isSocketOpen()) return; // Block
            socket.send(`${convert("NEW_LOG_RCVD")}/${0x00}/${getID()}`); 
        })
        .catch(err => console.log(err));
    }

    // Opens and closes log in the log display div ele.
    const openLog = () => { // Opens the log for display.
        const button = document.getElementById("logBut");
        const logDisp = document.getElementById("log");

        if (Flags.openLog) { // flag serves as a toggle to open and clear.
            let html = "";
            log.forEach(entry => html += `${entry}<br>`);
            logDisp.innerHTML = html;
            button.innerText = "Close log";
            Flags.openLog = false;

        } else {
            logDisp.innerText = ""; // Clear out.
            button.classList.remove("good"); // Reset back to default.
            button.innerText = "Open log";
            Flags.openLog = true;
        }
    }

    // Qty 1 - 12. Default to 6. Gets the previous n hours of temp/hum or
    // light trends.
    let getTrends = (qty = 6) => {
        if (!isSocketOpen()) return; // Block
        qty = (qty < 1 || qty > 12) ? 6 : qty; // Ensure within bounds, def set.
        socket.send(`${convert("GET_TRENDS")}/${qty}/${getID(setTrends)}`); 
    }

    // Requires bool from int to percent, and the value. Computes the perc
    // to int_16t value, or the int16_t to percentage from the analog to 
    // digital converter.
    let ADCConv = (toPerc, val) => {
        // Represents 32767 divided up into n steps.
        const steps = ADC_MAX / ADC_STEPS; // Converts to 32.767 @ 1000 steps

        // If going from integer to percent, mult steps by 10 to return a % val
        // that allows the percentage to be put into larger steps EX: 327.67  
        // that gives granularity to the tenth spot, or thousandth spot if 
        // decimal. EX: 45.<1>% or 0.45<1>.
        if (toPerc) return Number((val / (steps * 10)).toFixed(1));

        // Not to percent, percent to int. Ran tests on floor and no problems
        // occured when going from percent to int back to percent.
        const reduction = val * 10; // Put percentage into usable step qty.
        const final = Math.floor(reduction * steps); // % such as 45.1

        return final > 0 ? final : 0; // Ensures 0 is the minimum number.
    }

    // requires the intensity bar ID, the current value, the max value, the
    // starting color, and ending color. Colors the gradient background to show
    // graphical proportionality of value compared to its max.
    let intensityBar = (barID, val, max, clr1, clr2) => {
        const bar = document.getElementById(barID);
        const barWidth = bar.offsetWidth - 4; // -4 due to 2px borders. 
        const pxStop = Number(val) / Number(max) * Number(barWidth); // pixel

        // Set gradient of bar based on pixel stop threshold.
        bar.style.background = `linear-gradient(to right, 
            ${clr1} 0px, ${clr1} ${pxStop}px, 
            ${clr2} ${pxStop}px, ${clr2} ${barWidth}px)`;
            
        // Adjust any markers as well as the intensity bar. Dyanmic response.
        // If expansion seem to show skewing, its because it is proportionality
        // based, meaning a longer bar will show issues of improper placement.
        const MKR = Markers[barID]; // Arrays only

        if (MKR && MKR.length > 0) { // If marker exists and has elements.
            MKR.forEach(pos => {
                const marker = document.getElementById(`${barID}${pos}`);
                const pxl = Math.floor(Number(barWidth) * pos);
                marker.style.left = Number(pxl - 17) + "px"; // adj mkr width.
            });
        }
    }

    // Requires the intensity bar ID, and if init. Only called once, but left
    // open to modularity. Sets event listeners and builds all previously 
    // stored markers in local storage, upon init.
    let intensityBarMarker = (barID, init = false) => {
    
        const bar = document.getElementById(barID);
        const brdr = 2; // pixel width of the border set by css.
        const mrkr = 17; // pixel at 1/2 marker width.
        let MKR = Markers[barID]; // Will be array only.

        // First check global object to see if anything exists.
        if (!MKR) { // Nothing exists, use local.
            const loc = localStorage.getItem(`MKR${barID}`);

            if (!loc) { // No local storage exists.
                localStorage.setItem(`MKR${barID}`, "[]"); // Create empty set.
            } else { // Local storage exists.
                MKR = JSON.parse(loc); // Copy loc strg to global.
            }
        }

        // Requires the height, width, and relative X-pos. Can be called during
        // init, as well as local storage iteration. Builds and populates mkrs.
        let buildMarker = (height, width, relX) => {
            const marker = document.createElement("div");
            marker.classList.add("marker");
            marker.style.top = 0; // const
            marker.style.height = height + "px";
            marker.style.left = `${relX - mrkr - brdr}px`; // cursor = center
            marker.timeout = null; // Added with event list.

            // get marker placement percentage of bar based on mouse click loc.
            const perc = Number(((relX - brdr) / width).toFixed(5));
            
            marker.percentage = perc; // created percentage along inten bar.
            marker.id = `${barID}${perc}`;

            if (Markers[barID]) {Markers[barID].push(perc);} // exists.
            else {Markers[barID] = [perc];} // first creation

            localStorage.setItem(`MKR${barID}`, JSON.stringify(Markers[barID])); 

            marker.addEventListener("pointerdown", (e) => {
                bar.block = true; // Block addition of new marker to bar.
                marker.classList.add("selected"); // Highlight background.

                marker.timeout = setTimeout(() => { // Allows deletion with hold
                    bar.removeChild(marker);

                    // Remove marker from the markers object.
                    const idx = Markers[barID].indexOf(marker.percentage);
                    if (idx != -1) {
                        Markers[barID].splice(idx, 1); // Remove value 
                        localStorage.setItem(`MKR${barID}`, 
                            JSON.stringify(Markers[barID])); 
                    }
                    bar.block = false; // Allow new markers.
                }, 500);
            });

            marker.addEventListener("pointerup", (e) => {
                bar.block = false; // Allow new markers.
                marker.classList.remove("selected");
                clearTimeout(marker.timeout);
            });

            bar.appendChild(marker); // Set marker on bar.
        }

        if (init) { // First build, sets all ev list
            bar.classList.add("intensityBar");
            bar.id = barID;
            bar.block = false; // Used to block a new marker when it is sel.

            bar.addEventListener("pointerdown", (e) => {
                if (bar.block) return; // Block new marker creation if is sel.

                const xPos = e.clientX; // screen xpos.
                const rect = bar.getBoundingClientRect();
                const relX = xPos - rect.left; // Relative x position to bar.

                // height and width adjusted to inner dimensions of bar. Allows
                // size response if dimensions change.
                const height = bar.offsetHeight - (brdr * 2); // bar height
                const width = bar.offsetWidth - (brdr * 2); // bar width
                buildMarker(height, width, relX);
            });
        }

        // Recheck for data
        if (MKR && MKR.length > 0) { // populated, iterate and append.
            MKR.forEach(pos => {
                const height = bar.offsetHeight - (brdr * 2); // bar height
                const width = bar.offsetWidth - (brdr * 2); // bar width
                const pxl = Math.floor(Number(width * pos));

                // brdr added, since it will be subtracted in the build Maker.
                // Prevents a -2 subtraction with each load.
                buildMarker(height, width, pxl + brdr); 
            });
        } // No data otherwise, do nothing.
    }

    // Set the trends when received back from socker server.
    let setTrends = (data) => {
        const parent = document.getElementById("trendDisp");
        parent.innerText = ""; // Clear out before populating.
        const mainLab = document.createElement("div"); // main label.
        mainLab.classList.add("span4");
        mainLab.innerHTML = `<b><- - - Newer --- Older - - -></b><br>`;
        mainLab.style.textAlign = "center";
        parent.appendChild(mainLab);

        // ATTENTION !!!
        // Rules for data manipulation to includes units or percentages. Sent
        // by JSON from esp. If keys change, this must be updated as well.
        // Used for display purposes only in iteration below.
        const tempHumRULE = ["temp", "hum"]; // C/F, %

        // 15-bit value.
        const ADCRULE = ["photo", "soil_0", "soil_1", "soil_2", "soil_3"]; 
        const specRULE = ["blue", "clear", "cyan", "green", "indigo", "nir", 
            "orange", "red", "violet", "yellow"]; // 16-bit value

        // Iter JSON to display readings of sensor trends.
        Object.keys(data).forEach(key => {
            if (key === "id") return; // Skip AKA continue. Omit this kv pair.
            const lab = document.createElement("div");
            const content = document.createElement("div");
            lab.innerHTML = `<b>${key}</b>`;
            lab.style.textAlign = "center";
            lab.classList.add("span4");
            content.classList.add("span4");
            parent.appendChild(lab);
            parent.appendChild(content);

            let str = "";
            data[key].forEach((val, idx)=> { // Iterate and concat values.

                // Check rule JSON by key to convert and/or use % and units.
                if (tempHumRULE.includes(key)) {
                    if (key === "temp" && isCelcius) str += `[${val}C] , `;
                    else if (key === "temp" && !isCelcius) str += 
                        `[${((val * 1.8) + 32).toFixed(2)}F] , `;
                    else if (key === "hum") str += `[${val}%] , `;

                } else if (ADCRULE.includes(key)) {
                    str += `[${ADCConv(true, val)}%] , `;

                } else if (specRULE.includes(key)) {
                    const newV = ((val * 100) / SPEC_MAX).toFixed(2);
                    str += `[${newV}%] , `;
                    console.log(`SPEC ${key}`);
                }
            });

            str = str.slice(0, str.length - 3); // Removes comma and whitespace.
            content.innerText = str;
        });

        let close = document.createElement("button");
        close.innerText = "Close Trends";
        close.classList.add("span4");
        close.onclick = () => parent.innerText = ""; // Reset to blank.
        parent.appendChild(close);
    }

    // Relay number is used to display the appropriate value to input, which is
    // a number or NONE. Display purposes only.
    let relayNum = (val) => val === RE_OFF ? "NONE" : Number(val);

    // Used for RO display. Default is ADC is set to false, set to true to 
    // display ADC as a percentage vs int.
    let reAltCond = (cond, val, isADC = false) => { // Used for RO display
        const textConv = ["<", ">", "NONE"];
        if (cond == 2) return textConv[cond];
        if (isADC) return `${textConv[cond]} ${ADCConv(true, val)}%`;
        return `${textConv[cond]} ${val.toFixed(1)}`; // Non ADC value.
    }

    // Pings the server to see if new firmware is available. If the server 
    // has a fw version different than the device, it will reply with the
    // new version, which will allow a clickable update.
    const checkNewFW = () => {
        return new Promise((resolve, reject) => {
        fetch(OTAURL)
        .then(res => res.json())
        .then(res => {
            const {version} = res; // all valid responses will inc JSON version.
            const OTAdisp = document.getElementById("otaUpd");
            // All array items are what is expected from esp32 server.
            const noActionResp = ["Invalid JSON", "match", "wap", 
                "Connection open fail", "Connection init fail"];
                
            if (version && !noActionResp.includes(version)) {
                const html = `<button id="FWbut" class="logFWbut" 
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

        const OTAbutton = document.getElementById("FWbut");
        const OTAdisp = document.getElementById("otaUpd");

        fetch(URL)
        .then(res => res.json())
        .then(res => {
            const {status} = res;
            if (status === "OK") { // Exp response from the server.
                let secToReload = 10; // Reloads after firmware update.
                let intervalID = setInterval(() => { // Countdown to reload
                    OTAbutton.innerText = `Restarting in ${secToReload--}`;
                    if (secToReload < 1) {
                        clearInterval(intervalID);
                        window.location.reload();
                    }
                }, 1000);

            } else {
                window.alert("OTA update failed");
                OTAdisp.innerText = ""; // Clear out after fail.
            }
        })
        .catch(err => {
            window.alert("OTA update error");
            OTAdisp.innerText = ""; // Clear out after error.
        });
    }

    // Saves and resets the esp32 without countdown to reload window.
    const saveReset = () => {
        socket.send(`${convert("SAVE_AND_RESTART")}/0/${getID()}`);
        const disp = document.getElementById("SR"); // Save and reset button
        disp.onclick = ""; // Blocks further clicking until reset.
        let secToReload = 30; // Reloads after firmware update.
        let intervalID = setInterval(() => { // Countdown to reload
            disp.innerText = `Restarting in ${secToReload--}`;
            if (secToReload < 1) {
                clearInterval(intervalID);
                window.location.reload();
            }
        }, 1000);
    }

    // When called, sets a timeout for 15 sec to check for OTA updates,
    // and start interval to update after n amount of time.
    let checkUpdates = () => {

        let runCheck = (curTime) => { // Runs 
            if (MODE != 2) return; // Prevent check if not in station mode.
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

    // Opens the journal and notes section allowing the user to keep data about
    // the growing progress.
    let openJrnl = () => {
        const jrnlBut = document.getElementById("jrnlBut");
        const jrnlDisp = document.getElementById("jrnl");
        
        if (jrnlBut.isOpen) { // Currently open
            const entry = document.getElementById("jrnlEntry");
            jrnlBut.isOpen = false;
            jrnlBut.innerText = "Open Journal/Notes";
            localStorage.setItem("JRNL", entry.value);
            entry.remove();
            
        } else { // Currently closed.
            jrnlBut.isOpen = true;
            jrnlBut.innerText = "Close Journal/Notes";
            const entry = document.createElement("textarea");
            entry.classList.add("span4", "subConItem", "jrnlText");
            entry.id = "jrnlEntry";
            jrnlDisp.appendChild(entry);

            const textEntry = localStorage.getItem("JRNL");
            const dtg = new Date().toLocaleString();

            entry.value = textEntry ? textEntry : `Journal/Notes Opened ${dtg}`;
        }
    }

    // START CONTAINER OBJECTS. ================================================
    const mainCon = buildCon("main", null, null, null, null, null, null, null,
        {
            "units": new rangeBuild(0, 1, 1), "hour": new rangeBuild(0, 23, 1),
            "min": new rangeBuild(0, 59, 1)
        }
    );

    const tempCon = buildCon("temp", 
        new cmdBuild("ATTACH_RELAYS", handleTempHum),
        new cmdBuild("SET_TEMPHUM", handleTempHum),
        new cmdBuild("SET_TEMPHUM", handleTempHum), null, null, null, null, 
        {
            "re": new rangeBuild(0, 4, 1), "reCond": new rangeBuild(0, 2, 1),
            "reSet": new rangeBuild(-30, 60, 0.5), // Celcius
            "altCond": new rangeBuild(0, 2, 1),
            "altSet": new rangeBuild(-30, 60, 0.5) // Celcius
        }, true, true
    );

    const humCon = buildCon("hum", 
        new cmdBuild("ATTACH_RELAYS", handleTempHum),
        new cmdBuild("SET_TEMPHUM", handleTempHum),
        new cmdBuild("SET_TEMPHUM", handleTempHum), null, null, null, null,
        {
            "re": new rangeBuild(0, 4, 1), "reCond": new rangeBuild(0, 2, 1),
            "reSet": new rangeBuild(1, 99, 1), 
            "altCond": new rangeBuild(0, 2, 1),
            "altSet": new rangeBuild(1, 99, 1)
        }, true, true
    );

    const soilBuilder = (num) => {
        const soilCon = buildCon(`soil${num}`, null, null,
            new cmdBuild("SET_SOIL", handleSoil), null, null, null, null,
            {
                "altCond": new rangeBuild(0, 2, 1),
                "altSet": new rangeBuild(0.1, 99.9, 0.1)
            }, true, true, true
        );

        return soilCon;
    }

    const soil1Con = soilBuilder(0);
    const soil2Con = soilBuilder(1);
    const soil3Con = soilBuilder(2);
    const soil4Con = soilBuilder(3);

    const lightCon = buildCon("light", 
        new cmdBuild("ATTACH_RELAYS", handlePhoto),
        new cmdBuild("SET_LIGHT", handlePhoto), null, 
        new cmdBuild("SET_LIGHT", handlePhoto), null, null, null,
        {
            "re": new rangeBuild(0, 4, 1), "reCond": new rangeBuild(0, 2, 1),
            "reSet": new rangeBuild(0.1, 99.9, 0.1), 
            "darkSet": new rangeBuild(0.1, 99.9, 0.1), 
        }, true, true, true
    );

    const specCon = buildCon("spec", null, null, null, null, 
        new cmdBuild("SET_SPEC_INTEGRATION_TIME", handleSpec),
        new cmdBuild("SET_SPEC_GAIN", handleSpec), null,
        {
            "astep": new rangeBuild(0, 65534, 1),
            "atime": new rangeBuild(0, 255, 1),
            "again": new rangeBuild(0, 10, 1)
        }, true, true
    );

    const relayBuilder = (num) => { // Do not change naming convention with re.
        const relayCon = buildCon(`re${num}`, null, null, null, null, null, 
            null, // Relay control is within the buildRelayButtons
            new cmdBuild("RELAY_TIMER", handleRelays),
            {
                "reHour": new rangeBuild(0, 23, 1),
                "reMin": new rangeBuild(0, 59, 1),
                "reCont": new rangeBuild(0, 3, 1), 
                "reTimer": new rangeBuild(0, 86399, 1),
                "reDur": new rangeBuild(1, 1439, 1)
            }, true
        );

        return relayCon;
    }

    const re1Con = relayBuilder(0);
    const re2Con = relayBuilder(1);
    const re3Con = relayBuilder(2);
    const re4Con = relayBuilder(3);

    const allCon = [mainCon, tempCon, humCon, soil1Con, soil2Con, soil3Con, 
        soil4Con, lightCon, specCon, re1Con, re2Con, re3Con, re4Con];

    // END CONTAINER OBJECTS. ==================================================
    // START OPENERS AND BUILDERS. =============================================

    const buildContainers = () => allCon.forEach(con => buildRO(con));

    const addListeners = () => {
        allCon.forEach(con => {
            document.getElementById(con.buttonID).onclick = () => expand(con);
        });
    }

    let loadPage = () => { // Upon load, build everything.
        buildContainers();
        buildSpec();
        buildTrends();
        addListeners();
        initWebSocket(); // Inits the websocket protocol.
        getLog(); // Gets the log when loading the page.
        checkUpdates(); // Starts firmware checking timeout and interval.
    }

    // END OPENERS AND BUILDERS. ===============================================

    </script>
    
</body>
</html>
)rawliteral";

}

#endif // WEBPAGES_HPP