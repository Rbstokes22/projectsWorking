const fs = require("fs");
const os = require("os");
const path = require("path");

// Checks the RAMDISK path, returns path if exists. If not-exists, creates
// a fallback tempdir that is not RAMDISK but is temporary.
let resolveRAMPath = () => {

    const ramPaths = [
        "/ramdisk", // Dockers tmpfs mount and primary target
        "/dev/shm", // Guaranteed RAM on Linux hosts
    ];

    for (const p of ramPaths) {
        if (fs.existsSync(p)) {
            console.log("RAMDISK ACTIVE");
            return p;
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

    return fallback;
}

module.exports = {resolveRAMPath};