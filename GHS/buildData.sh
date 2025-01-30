#!/bin/bash
URL="shadyside.local"
PORT="5702"
pvt_key_path="buildData/keys/private_key.pem"
pub_key_path="buildData/keys/public_key.pem"
pvt_key_bits=2048
primary_sig_path="data/app0firmware.sig"
secondary_sig_path="data/app1firmware.sig"
firmware_location=".pio/build/esp32dev/firmware.bin"
signatureDest="/home/shadyside/Desktop/Programming/testServer/firmware/signature"
firmwareDest="/home/shadyside/Desktop/Programming/testServer/firmware/firmware"

# Generate keys if they do not exist
if [ ! -f "${pvt_key_path}" ]; then 
    openssl genpkey -algorithm RSA -out "${pvt_key_path}" -pkeyopt rsa_keygen_bits:"${pvt_key_bits}"
    echo "Private Key Created in buildData/keys"
fi 

if [ ! -f "${pub_key_path}" ]; then 
    openssl rsa -pubout -in "${pvt_key_path}" -out "${pub_key_path}"
    echo "Public Key Created in buildData/keys. Copy public key into firmwareVal.cpp"
fi 

# Init conda
source /home/shadyside/anaconda3/bin/activate

# Compiles the firmware.bin
if ! pio run; then 
    echo "Error compiling firmware"
    exit 1
fi

# Once compiled you will see the byte size based on the size of the private
# key used. 
firmware_size=$(stat -c %s "${firmware_location}")
signature_size=$((pvt_key_bits / 8)); 
echo "Update config.hpp FIRMWARE_SIZE = ${firmware_size}, FIRMWARE_SIG_SIZE = ${signature_size}"

# prompts use to save the updated size
read -p "Press enter once size is updated and saved"

# Recompiles with the updated size 
if ! pio run; then 
    echo "Error recompiling firmware"
    exit 1
fi

# The file is ready to be written to esp32. Generates the signatures and writes
# them as app0firmware.sig. Generates the crc of the signature and appends
# it to the file. Copies the data to app1firmware.sig to be uploaded together.
# The signature will be 256 bytes + 4 bytes for the checksum. Using -sha256 says
# first hash this document using -sha256, and then sign it using that key. The file
# size will always be the size of the key, in this case a 2048 is 256 bytes. 
openssl dgst -sha256 -sign "${pvt_key_path}" -out "${primary_sig_path}" "${firmware_location}" 
cp "${primary_sig_path}" "${secondary_sig_path}"

# run a prompt to automatically write
read -p "a) USB write b) LAN OTA write c) manual write: " response

if [[ "${response}" == [aA] ]]; then 
    echo "Proceeding to write to the ESP via USB..."
    pio run --target uploadfs && pio run --target upload 

elif [[ "${response}" ==  [bB] ]]; then
    echo "Proceeding to write to the ESP via OTA..."
    isRunning=$(curl -s http://"${URL}:${PORT}")

    if [ "${isRunning}" != "Success" ]; then 
        node buildData/OTAserver/server.js &
        serverPID=$!
        echo "Server started with PID: ${serverPID}"
    fi

    # Sleep for 3 seconds
    sleep 3
    otaURL=$(curl -s http://"${URL}:${PORT}"/IP)

    echo "Running http://greenhouse.local/OTAUpdateLAN?url=http://${otaURL}:${PORT}"
    curl "http://greenhouse.local/OTAUpdateLAN?url=http://${otaURL}:${PORT}"
    kill "${serverPID}"

    # Server self terminates

else
    echo "You must manually write to the ESP."

    read -p "Would you like to send the signature and firmware to the server? (y/n)" copyData

    if [[ "${copyData}" == [yY] ]]; then
        read -p "What version is your firmware? " version
        
        echo "Copying signature to ${signatureDest}"
        cp "${primary_sig_path}" "${signatureDest}"/S"${version}".sig

        echo "Copying firmware to ${firmwareDest}"
        cp "${firmware_location}" "${firmwareDest}"/F"${version}".bin

        echo "Complete"
    fi 

fi
