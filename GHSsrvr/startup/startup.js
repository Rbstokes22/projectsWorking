const fs = require("fs");
const os = require("os");
const path = require("path");
const Bonjour = require("bonjour-service").default; // mDNS
const {config} = require("../config/config");

// Checks the RAMDISK path. Populates config.RAM_PATH upon running this func.
let resolveRAMPath = () => {

    const ramPaths = [
        "/ramdisk", // Dockers tmpfs mount and primary target
        "/dev/shm", // Guaranteed RAM on Linux hosts
    ];

    for (const p of ramPaths) {
        if (fs.existsSync(p)) {
            console.log("RAMDISK ACTIVE");
            config.RAM_PATH = p;
            return 1;
        }
    }

    // If no directories exist, attempt to create a fallback temp dir.
    const fallback = path.join(os.tmpdir(), "ramdisk");

    try {

        fs.mkdirSync(fallback, {recursive: true});
        console.log("RAMDISK NOT ACTIVE USING FALLBACK");

    } catch (e) {
        
        console.log("RAMDISK NOT CREATED BAD FALLBACK");
    }

    config.RAM_PATH = fallback;
}

// Requires no params. Starts mDNS service and listener when called.
let startmDNS = () => {

    const bonjour = new Bonjour();
    
    bonjour.publish(config.mDNSConfig);

    process.on("SIGINT", () => {
        console.log("Unpublishing mDNS services and shutting down");
        bonjour.unpublishAll(() => {
            bonjour.destroy();
            process.exit();
        });
    });
}

module.exports = {resolveRAMPath, startmDNS};