const DEV_FINDER_POLL_INT = 2000; // Used to look at all active devices

let populateDevices = (devMap) => {
    const dispID = document.getElementById("availableDev");
    dispID.innerText = ""; // clear old data upon each run.
    const regex = //

    Object.keys(devMap).forEach(device => {

        let button = document.createElement("button");

        // Strip the button of the entire URL and just include the device 
        // name.
        let re = /^http:\/\/(greenhouse\d*)\.local$/;

        button.innerText = re.exec(device)[1];

        button.onclick = () => {
            console.log(`${device} clicked`);
        }

        dispID.appendChild(button);
    });

}

let findActive = setInterval(() => {

    fetch(`http://${url}/getActive`)
    .then(response => response.json())
    .then(response => {
        
        populateDevices(response);
       
    }).catch(err => console.error(err));

}, DEV_FINDER_POLL_INT);