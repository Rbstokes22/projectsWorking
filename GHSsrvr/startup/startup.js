const fs = require("fs");
const os = require("os");
const path = require("path");

let resolveRAMPath = () => {

    const ramPaths = [
        "/ramdisk", // Dockers tmpfs mount and primary target
        "/dev/shm", // Guaranteed RAM on Linux hosts
        path.join(os.tempdir(), "ramdisk") // Last resort not RAM.
    ];

    for (const p of ramPaths) {
        try {
            if (fs.existsSync(p)) {
                return p;
            }
        } catch (e) {}
    }

    // If no directories exist, attempt to create a fallback temp dir.
    const fallback = path.join(os.tempdir(), "ramdisk");
    try {
        fs.mkdirSync(fallback, {recursive: true});
    } catch (e) {}

    return fallback;
}