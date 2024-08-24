#!/bin/bash

# Compiles the firmware.bin
pio run 

# Once compiled you will see the byte size
firmware_size=$(stat -c %s .pio/build/esp32dev/firmware.bin)
echo "Update firmwareVal.cpp FIRMWARE_SIZE with $firmware_size"

# prompts use to save the updated size
read -p "Press enter once size is updated and saved"

# Recompiles with the updated size 
pio run

# The file is ready to be written to esp32. Generate signatures and backups, along
# with checksums.
openssl dgst -sha256 -sign buildData/keys/private_key.pem -out data/firmware.sig .pio/build/esp32dev/firmware.bin 
cp data/firmware.sig data/firmwareBU.sig
./buildData/gencrc

# run a prompt to automatically write
read -p "Would you like to write to the ESP at this time? (y/n): " response

if [[ "$response" == [yY] ]]; then 
    echo "Proceeding to write to the ESP..."
    pio run --target uploadfs 
    pio run --target upload 
else
    echo "You must manually write to the ESP."
fi
