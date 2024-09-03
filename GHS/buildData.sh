#!/bin/bash

signatureDest="/home/shadyside/Desktop/Programming/testServer/firmware/signature"
firmwareDest="/home/shadyside/Desktop/Programming/testServer/firmware/firmware"

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
echo "Update config.hpp FIRMWARE_SIZE = $firmware_size, FIRMWARE_SIG_SIZE = $signature_size"

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
read -p "a) USB write b) LAN OTA write c) manual write: " response

if [[ "$response" == [aA] ]]; then 
    echo "Proceeding to write to the ESP via USB..."
    pio run --target uploadfs 
    pio run --target upload 

elif [[ "$response" ==  [bB] ]]; then
    echo "Proceeding to write to the ESP via OTA..."
    isRunning=$(curl -s http://localhost:5555)

    if [ "$isRunning" != "Success" ]; then 
        node buildData/OTAserver/server.js &
        serverPID=$!
        echo "Server started with PID: ${serverPID}"
    fi

    # Sleep for 3 seconds
    sleep 3
    otaURL=$(curl -s http://localhost:5555/IP)

    echo "Running http://greenhouse.local/OTAUpdateLAN?url=http://${otaURL}:5555"
    curl "http://greenhouse.local/OTAUpdateLAN?url=http://${otaURL}:5555"
    kill $serverPID

    # Server self terminates

else
    echo "You must manually write to the ESP."

    read -p "Would you like to send the signature and firmware to the server? (y/n)" copyData

    if [[ "$copyData" == [yY] ]]; then
        read -p "What version is your firmware? " version
        
        echo "Copying signature to ${signatureDest}"
        cp data/app0firmware.sig ${signatureDest}/S${version}.sig

        echo "Copying firmware to ${firmwareDest}"
        cp .pio/build/esp32dev/firmware.bin ${firmwareDest}/F${version}.bin

        echo "Complete"
    fi 

fi
