// PARTITIONS

The esp flash is standard at 1MB. In order to override this, there is a 
partitions_custom.csv in the root directory that partitions everything out.
This provides a significant increase in flash memory. Ensure to include the 
.csv file in the platformio.ini file like this 

board_build.partitions = partitions_custom.csv

// Digital signature of firmware.bin

The buildData directory will store the keys in the keys directory, as 
well as the crc32 program that will generate a signature based on the 
firmware.bin file. Ensure you do not delete these keys, and you include
the public key in the firmwareVal.cpp

Build the data directory to upload to fs. Run from the root dir, 
./buildData, that will create keys if non-existant, as well as 
two firmware signatures for each partition. Once everything is 
built, it will flash everything to the esp32.

// MDNS

The MDNS library location changed and it was problematic for this system.
used  git clone https://github.com/espressif/esp-protocols.git, and then
created a components dir in root folder. Moved the mdns dir to the components
and deleted the rest. Updated the cmakelist to REQUIRE mdns and updated the 
c_cpp_properties.json to include the mdns path. This allowed me to include it 
in the files.

// SPIFFS used in OTA updates and firmware validation.

ADD TO MAIN TO REGISTER

    esp_vfs_spiffs_conf_t spiffsConf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t spiffsErr = esp_vfs_spiffs_register(&spiffsConf);
    if (spiffsErr != ESP_OK) {
        printf("FAILED TO MOUNT SPIFFS\n");
    }

TO UNREGISTER

esp_vfs_spiffs_unregister(NULL); Not unregistering in this program due to OTA.

// HTTPS SERVER

Not used. Tried to implement but there is no way to get around 
the self signed certificate, and a valid CA will not authorize 
for a device used on a local network. Will use http since it is on
the lan.

// OTA

Had to use the esp_crt_bundle.h that became available by enabling 
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE in the menuconfig. This is required 
for http clients when pinging an https server to validate server cert.

The OTA will always try to update from the LAN first. This is only for 
local updates for developing purposes. If the OTA server is running, it 
will update, if not it will default to web.

To update the OTA via LAN, ensure that the server OTAserver is running,
and the OTA path is correct in the server.js. It will then print its 
URL. You will then type in the address bar:
greenhouse.local/OTAUpdate?url=<SERVER URL>. It will look something
like this:
http://greenhouse.local/OTAUpdate?url=http://192.168.86.246:5555
The update will then commence

To update via Webpage, ...

// Socket Commands

All commands are in the format <uint8_t command>/<int value>/<message ID>

command: Passed to the socket indicating action
value: value used in the command
message ID: Sent back to client with info to allow client to execute callback
            based on that ID.

Below will be the commands and will be in a format like above. It will give
the command/value range if applicable or NR (not required)/ID

1/NR/ID: Gets all of the current data from the controller

2/0-86399/ID: Calibrates the controller time in seconds past midnight with 
    the client time. This allows all timers to execute at appropriate times.

3/0-3/ID: Changes the relay 1 status to off (0), on (1), forceoff(2), or 
    remove force (3). This is to control relays manually as opposed to having
    them set to specific sensors only. All devices using a relay have their
    ID in a list as being attached to that relay, and the relay cannot shut off
    until all of these IDs are removed. Using forceoff, will override and shut
    relay off despite other process using them. The force must be removed.

4/0-3/ID: Same as 3, applies to relay 2.
5/0-3/ID: Same as 3, applies to relay 3.
6/0-3/ID: Same as 3, applies to relay 4.



