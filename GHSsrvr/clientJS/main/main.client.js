const DEV_FINDER_POLL_INT = 2000; // Used to look at all active devices

// Populates all active devices on the html page. 
const populateDevices = (devMap) => {
    const dispID = document.getElementById("availableDev");
    dispID.innerText = ""; // clear old data upon each run.

    Object.keys(devMap).forEach(device => {

        let button = document.createElement("button");
        button.id = devMap[device].ip;
        button.innerText = devMap[device].name;

        if (!devMap[device].isUp) {
            button.className = "devUp";
            
            button.onclick = () => {

                window.location.href = `http://${url}/getDashboard?ip=` +
                    `${devMap[device].ip}&ws=${devMap[device].sockURL}`;
            }

        } else { // Device down, disable functionality and change color.
            button.className = "devDown";
            button.classList.remove();
        }

        dispID.appendChild(button);
    });
}

// Sets a interval that polls for all active peripheral devices communicating
// with the server. Upon discovery, populates them into display feature.
let findActive = setInterval(() => {

    fetch(`http://${url}/getActive`)
    .then(response => response.json())
    .then(response => {
        
        populateDevices(response);
       
    }).catch(err => console.error(err));

}, DEV_FINDER_POLL_INT);