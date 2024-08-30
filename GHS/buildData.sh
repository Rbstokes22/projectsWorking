#!/bin/bash

# Generate keys if they do not exist
if [ ! -f "buildData/keys/private_key.pem" ]; then 
    openssl genpkey -algorithm RSA -out buildData/keys/private_key.pem -pkeyopt rsa_keygen_bits:2048
    echo "Private Key Created in buildData/keys"
fi 

if [ ! -f "buildData/keys/public_key.pem" ]; then 
    openssl rsa -pubout -in buildData/keys/private_key.pem -out buildData/keys/public_key.pem
    echo "Public Key Created in buildData/keys. Copy public key into firmwareVal.cpp"
fi 

# Compiles the firmware.bin
pio run 

# Once compiled you will see the byte size
firmware_size=$(stat -c %s .pio/build/esp32dev/firmware.bin)
signature_size=$(stat -c %s data/app0firmware.sig);
echo "Update main.cpp FIRMWARE_SIZE = $firmware_size, FIRMWARE_SIG_SIZE = $signature_size"

# prompts use to save the updated size
read -p "Press enter once size is updated and saved"

# Recompiles with the updated size 
pio run

# The file is ready to be written to esp32. Generates the signatures and writes
# them as app0firmware.sig. Generates the crc of the signature and appends
# it to the file. Copies the data to app1firmware.sig to be uploaded together.
openssl dgst -sha256 -sign buildData/keys/private_key.pem -out data/app0firmware.sig .pio/build/esp32dev/firmware.bin 
./buildData/gencrc >> data/app0firmware.sig # appends checksum to signature file.
cp data/app0firmware.sig data/app1firmware.sig

# run a prompt to automatically write
read -p "Would you like to write to the ESP at this time? (y/n): " response

if [[ "$response" == [yY] ]]; then 
    echo "Proceeding to write to the ESP..."
    pio run --target uploadfs 
    pio run --target upload 
else
    echo "You must manually write to the ESP."
fi
