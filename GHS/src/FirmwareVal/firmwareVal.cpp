#include "FirmwareVal/firmwareVal.hpp"
#include "esp_partition.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
// Get MSG LOG ERR in here for OLED

namespace Boot {

// !!! The firmware size must be entered correctly for an appropriate
// hash value. This will be obtained by running buildData.sh from the 
// root, and will walk you through the upload process.
const size_t FIRMWARE_SIZE = 1108944;

const char* pubKey = R"rawliteral(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA8D+FXkYt9VnNbFXYJ1zn
0wi4ARgpA7n2aDrJxj44pK77zyPEjkYYrQ2d1uHegHaag4g4NGsGw5HxIo54Uwb7
q4T0i0J42FFD34sjwGNOqF56gQiTh5ELrVmuVpYCQOIwUIV5Hs/z+G2pKFVdPpmH
DPj81fYnNsd5XLCK1ClWqf5NpPlSm6r8Y6tpU30pjQErkychV6o6HlgWsU3Ut9x5
BGy5YrsqVUImN6uct4wMiPewYv1PaSVk3ABOVxQ33barx5rJtOfoyPfvz/zG8348
gKIl1ssMaWQvgrJtnDoZWL+1jyatol0pnX9YrY0crbyGWSkDHH+PfOBkoiukVad5
5wIDAQAB
-----END PUBLIC KEY-----)rawliteral";

bool CSsafe = false;

// Takes in a pointer to the unit8_t array, and its size in values,
// and returns the uint32_t checksum. 
uint32_t computeCS(const uint8_t* data, size_t size) {

    // If bad data is sent to checksum, returns a max value and
    // sets the flag for bad data.
    if (data == nullptr || size == 0) {
        CSsafe = false; 
        return 0xFFFFFFFF;

    } else {

        // Creates a pointer that points to the bytes of the data
        // by casting the pointer as a uint8_t type.
        const uint8_t* bytes = static_cast<const uint8_t*>(data);

        // Good data was sent, so flag is set to true.
        CSsafe = true;

        // Inits the crc32 at max value to ensure all leading bits 
        // are set which helps process data starting with zeros.
        uint32_t crc32 = 0xFFFFFFFF;

        // Each byte is iterated for the size passed, which is why
        // the correct size is crucial.
        for(size_t addr = 0; addr < size; addr++) {

            // The crc32 is then xor with the byte data, which is a method
            // to integrate the bytes into the crc32 in a non destructive 
            // way. Everything is reversable and bits. This iteration
            // goes bit by bit shifting  or shifting and xoring with the 
            // polynomial when the least significant bit == 1. 
            crc32 ^= bytes[addr];

            for(size_t bit = 0; bit < 8; bit++) {
                if (crc32 & 1) {
                    crc32 = (crc32 >> 1) ^ 0xEDB88320;
                } else {
                    crc32 >>= 1;
                }
            }
        }

        // Ensures it does not return 0 with a 0 input.
        return crc32 ^ 0xFFFFFFFF;
    }
}

// Accepts uint8_t pointer that the signature will be copied to, as 
// well as the size. Copies the spiffs file signature to the passed
// pointer. Checks integrity with a crc32 value, and returns SIG_OK
// for the primary signature, SIG_OK_BU if the primary fails but the 
// BU is ok, or SIG_FAIL if unable to verify signatures.
size_t getSignature(uint8_t* signature, size_t size) {
    size_t bytesRead{0};
    size_t sigLen{0};
    uint32_t storedCS{0};
    uint8_t iteration{0}; // used to check if primary or BU
    
    Files files[] = {
        {"/spiffs/firmware.sig", "/spiffs/firmwareCS.bin"},
        {"/spiffs/firmwareBU.sig", "/spiffs/firmwareBUCS.bin"}
    };

    for (auto &file : files) {
        FILE* f = fopen(file.dir, "rb");

        if (f == NULL) {
            printf("Unable to open file: %s\n", file.dir);
        }

        // The number 1 indicates you want to read byte by byte.
        sigLen = bytesRead = fread(signature, 1, size, f);
        uint32_t CS = computeCS(signature, bytesRead);

        fclose(f);

        f = fopen(file.CS, "rb");

        if (f == NULL) {
            printf("Unable to open CS file: %s\n", file.CS);
        }

        // The CSbufSize indicates items of size 4, or uint32_t size.
        bytesRead = fread(&storedCS, sizeof(uint32_t), 1, f);
        if (bytesRead != 1) {
            printf("Failed to read checksum from file: %s\n", file.CS);
        }

        if (storedCS == CS && CSsafe) {
            return sigLen;
        } else {
            iteration++;
        }
    }

    return sigLen;
}

VAL readPartition(const esp_partition_t* partition, uint8_t* hash) { 
    size_t chunkSize = 1024;
    uint8_t buffer[chunkSize]{0};
    size_t FW = FIRMWARE_SIZE;

    // Init the SHA-256 context
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    if (mbedtls_sha256_starts(&ctx, 0) != 0) {
        printf("mbedtls failed to start\n");
        return VAL::PARTITION_FAIL;
    }

    for (size_t offset = 0; offset < FW; offset += chunkSize) {
        size_t toRead = (offset + chunkSize > FW) ? (FW - offset) : chunkSize;
        esp_err_t err = esp_partition_read(partition, offset, buffer, toRead);
        if (err != ESP_OK) {
            printf("Failed to read partition: %s\n", esp_err_to_name(err));
            mbedtls_sha256_free(&ctx);
            return VAL::PARTITION_FAIL;
        }

        if (mbedtls_sha256_update(&ctx, buffer, toRead) != 0) {
            printf("Error updating sha256\n");
            return VAL::PARTITION_FAIL;
        }
    }

    if (mbedtls_sha256_finish(&ctx, hash) != 0) {
        printf("Failed to finish sha256\n");
        return VAL::PARTITION_FAIL;
    }

    mbedtls_sha256_free(&ctx);

    return VAL::PARTITION_OK;
}

VAL verifySig(const uint8_t* firmwareHash, const uint8_t* signature, size_t signatureLen) {
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    // Load the public key
    if (mbedtls_pk_parse_public_key(&pk, (const unsigned char*)pubKey, strlen(pubKey) + 1) != 0) {
        printf("Failed to parse public key\n");
        return VAL::SIG_FAIL;
    }

    // Verify the signature
    int ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, firmwareHash, 32, signature, signatureLen);
    
    mbedtls_pk_free(&pk); // Free the public key context

    if (ret == 0) {
        printf("Signature is valid.\n");
        return VAL::SIG_OK; // Signature is valid
    } else {
        printf("Signature is invalid. Error code: %d\n", ret);
        return VAL::SIG_FAIL; // Signature is invalid
    }
}

VAL validateSig(const esp_partition_t* partition) {
    uint8_t hash[32]{0};
    uint8_t sig[300]{0};

    if (partition == NULL) {
        printf("Failed to allocate memory for partition data\n");
        return VAL::SIG_FAIL;
    }

    if (readPartition(partition, hash) == VAL::PARTITION_FAIL) {
        return VAL::SIG_FAIL;
    }

    // Get signature
    size_t sigLen = getSignature(sig, sizeof(sig));
    if (sigLen == 0) {
        return VAL::SIG_FAIL;
    }

    // Verify signature
    return verifySig(hash, sig, sigLen);
}

// checks partition and compares it against the signature of the firmware.
// Takes argument CURRENT or NEXT for the partition type, and returns 
// VALID or INVALID.
VAL checkPartition(PART type) {
    VAL ret{VAL::INVALID};

    // All error handling handled in callbacks.
    auto validate = [&ret](const esp_partition_t* part) {
        if (part != NULL) {
            if (validateSig(part) == VAL::SIG_OK) {
                ret = VAL::VALID;
            } 
        } 
    }; 

    if (type == PART::CURRENT) {
        validate(esp_ota_get_running_partition());

    } else if (type == PART::NEXT) {
        validate(esp_partition_find_first(
            ESP_PARTITION_TYPE_APP, 
            ESP_PARTITION_SUBTYPE_APP_OTA_0, 
            NULL));

    } else {
        printf("Must be a partition CURRENT or NEXT\n");
    }

    return ret;
}

}