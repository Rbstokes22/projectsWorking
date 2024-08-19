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

    </style>
</head>
<body>
    <button onclick="getOTA()">Get OTA</button>

    <script>

        let curURL = `${window.location.href}OTAUpdate`;

        const getOTA = () => {
            fetch(curURL);
        }

    </script>
    
</body>
</html>
)rawliteral";

}

#endif // WEBPAGES_HPP