// PARTITIONS

The esp flash is standard at 1MB. In order to override this, there is a 
partitions_custom.csv in the root directory that partitions everything out.
This provides a significant increase in flash memory. Ensure to include the 
.csv file in the platformio.ini file like this 

board_build.partitions = partitions_custom.csv

// OPENSSL

Create key
openssl genrsa -out server.key 2048

Create cert 
openssl req -new -x509 -key server.key -out server.crt -days 365

// MDNS

The MDNS library location changed and it was problematic for this system.
used  git clone https://github.com/espressif/esp-protocols.git, and then
created a components dir in root folder. Moved the mdns dir to the components
and deleted the rest. Updated the cmakelist to REQUIRE mdns and updated the 
c_cpp_properties.json to include the mdns path. This allowed me to include it 
in the files.

// SPIFFS (if used in the future)

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

esp_vfs_spiffs_unregister(NULL);

// HTTPS SERVER

Not used. Tried to implement but there is no way to get around 
the self signed certificate, and a valid CA will not authorize 
for a device used on a local network.

