#include "FirmwareVal/firmwareVal.hpp"
#include "esp_partition.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "UI/MsgLogHandler.hpp"

namespace Boot {

// This is generated using buildData/buildKeys.sh. The public key is 
// copied into here.
const char* pubKey = R"rawliteral(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA8D+FXkYt9VnNbFXYJ1zn
0wi4ARgpA7n2aDrJxj44pK77zyPEjkYYrQ2d1uHegHaag4g4NGsGw5HxIo54Uwb7
q4T0i0J42FFD34sjwGNOqF56gQiTh5ELrVmuVpYCQOIwUIV5Hs/z+G2pKFVdPpmH
DPj81fYnNsd5XLCK1ClWqf5NpPlSm6r8Y6tpU30pjQErkychV6o6HlgWsU3Ut9x5
BGy5YrsqVUImN6uct4wMiPewYv1PaSVk3ABOVxQ33barx5rJtOfoyPfvz/zG8348
gKIl1ssMaWQvgrJtnDoZWL+1jyatol0pnX9YrY0crbyGWSkDHH+PfOBkoiukVad5
5wIDAQAB
-----END PUBLIC KEY-----)rawliteral";

bool CSsafe = false; // checksum safe flag.
char log[LOG_MAX_ENTRY] = {0};
const char* tag = "(FirmWVal)";

// checks partition and compares it against the signature of the firmware.
// Takes argument CURRENT or NEXT for the partition type, and returns 
// VALID or INVALID.
val_ret_t checkPartition(PART type, size_t FWsize, size_t FWSigSize) {
    
    val_ret_t ret{val_ret_t::INVALID};

    // Print to serial.
    snprintf(log, sizeof(log), 
        "%s Checking partition size %zu with a sig size %zu", tag, FWsize, 
            FWSigSize);

    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        log, Messaging::Method::SRL);

    // All error handling handled in callbacks.
    auto validate = [&ret, FWsize, FWSigSize](const esp_partition_t* part) {
        if (part != NULL) {

            snprintf(log, sizeof(log), "%s Validating partition label: %s", 
                tag, part->label);

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
                log, Messaging::Method::SRL);

            if (validateSig(part, FWsize, FWSigSize) == val_ret_t::SIG_OK) {
                ret = val_ret_t::VALID;
            } 

        } else {
            snprintf(log, sizeof(log), "%s Partion label: %s = NULL", 
                tag, part->label);

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
                log, Messaging::Method::SRL_LOG);
        }
    }; 

    if (type == PART::CURRENT) {
        validate(esp_ota_get_running_partition());

    } else if (type == PART::NEXT) {
        validate(esp_ota_get_next_update_partition(NULL));
        
    } else {
        snprintf(log, sizeof(log), "%s Only partition CURRENT or NEXT", tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);
    }

    return ret;
}

// Validates the signature. Takes the partition, firmware size, and 
// firmware signature size. Reads the partition to generate a hash,
// Gets the firmware signature, and verifies the hash against the 
// signature hash. Returns SIG_OK or SIG_FAIL.
val_ret_t validateSig(
    const esp_partition_t* partition, 
    size_t FWsize, 
    size_t FWSigSize
    ) {

    uint8_t hash[32]{0};
    uint8_t sig[FWSigSize]{0}; 

    char label[20]{0}; // Gets label name either app0 or app1
    strcpy(label, partition->label);

    if (partition == NULL) {
        snprintf(log, sizeof(log), 
            "%s Failed to allocate memory for partition", tag);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);

        return val_ret_t::SIG_FAIL;
    }

    // Reads the partition and generates the hash.
    if (readPartition(partition, hash, FWsize) == val_ret_t::PARTITION_FAIL) {
        return val_ret_t::SIG_FAIL; // Logging in function call
    }

    // Get signature from spiffs.
    size_t sigLen = getSignature(sig, sizeof(sig), label);
    if (sigLen == 0) {
        return val_ret_t::SIG_FAIL; // Logging in function call
    }

    // Verify signature hash against firmware hash.
    return verifySig(hash, sig, sigLen);
}

// Reads the partition to generate a sha256 hash. Takes partition, hash array,
// and firmware size. Returns PARTITION_OK or PARTITION_FAIL.
val_ret_t readPartition(
    const esp_partition_t* partition, 
    uint8_t* hash, 
    size_t FWsize
    ) { 

    // Buffer used to read server response and write chunk data to flash.
    uint8_t buffer[PART_CHUNK_SIZE]{0};

    // Init the SHA-256 context
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    // Starts checksum computation
    if (mbedtls_sha256_starts(&ctx, 0) != 0) {
        snprintf(log, sizeof(log), "%s mbedtls failed to start", tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);

        return val_ret_t::PARTITION_FAIL;
    }

    // Iterates through the firmware partition in chunks. Updates the sha256 
    // hash as the iteration continues.
    for (size_t offset = 0; offset < FWsize; offset += PART_CHUNK_SIZE) {
        size_t toRead = (offset + PART_CHUNK_SIZE > FWsize) ? 
            (FWsize - offset) : PART_CHUNK_SIZE;

        // Reads partition.
        esp_err_t err = esp_partition_read(partition, offset, buffer, toRead);

        if (err != ESP_OK) { // partition read error.

            snprintf(log, sizeof(log), "%s Failed to read partition: %s", tag,
                esp_err_to_name(err));

            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
                log, Messaging::Method::SRL_LOG);

            mbedtls_sha256_free(&ctx); // If err, free sha256 context.
            return val_ret_t::PARTITION_FAIL;
        }

        // Feeds buffer into running sha256 computation.
        if (mbedtls_sha256_update(&ctx, buffer, toRead) != 0) {

            snprintf(log, sizeof(log), "%s Err updating sha256", tag);
            Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
                log, Messaging::Method::SRL_LOG);

            mbedtls_sha256_free(&ctx); // If err, free sha256 context.
            return val_ret_t::PARTITION_FAIL;
        }
    }

    // Once complete finishes the hash and writes result to hash buffer.
    if (mbedtls_sha256_finish(&ctx, hash) != 0) {

        snprintf(log, sizeof(log), "%s Failed to finish sha256", tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);

        mbedtls_sha256_free(&ctx); // If err, free sha256 context.
        return val_ret_t::PARTITION_FAIL;
    }

    mbedtls_sha256_free(&ctx); // Free sha256 context when done.

    return val_ret_t::PARTITION_OK;
}

// Accepts uint8_t pointer that the signature will be copied to, as 
// well as the size and partition label. Copies the spiffs file 
// signature to the passed pointer and returns the size of the signature.
size_t getSignature(uint8_t* signature, size_t sigSize, const char* label) {
    size_t bytesRead{0};
    uint8_t buffer[sigSize]; 
    char filepath[FW_FILEPATH_SIZE]{0};

    sprintf(filepath, "/spiffs/%sfirmware.sig", label); // either app0 or app1
    snprintf(log, sizeof(log), "%s Reading filepath: %s", tag, filepath);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        log, Messaging::Method::SRL);

    FILE* f = fopen(filepath, "rb"); // Opens with read bytes

    if (f == NULL) { // If null return
        snprintf(log, sizeof(log), "%s Unable to open file %s", tag, filepath);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);
    }

    // Reads the file into the buffer.
    bytesRead = fread(buffer, 1, sizeof(buffer), f);
    
    // Copies the signature portion of the buffer to the signature, and
    // the checksum portion to the storedCS value.
    memcpy(signature, buffer, sigSize); // 256 bytes for the signature
    fclose(f); // Close file when done.

    return bytesRead; // Signature length
}

// Takes the firmware hash, signature hash, and the signature length.
// Verifies that the hashes match. Returns SIG_OK or SIG_FAIL.
val_ret_t verifySig(
    const uint8_t* firmwareHash, 
    const uint8_t* signature, 
    size_t signatureLen
    ) {

    mbedtls_pk_context pk; // public key container
    mbedtls_pk_init(&pk);

    // Load the public key from this source file.
    if (mbedtls_pk_parse_public_key(&pk, (const unsigned char*)pubKey, 
        strlen(pubKey) + 1) != 0) {

        snprintf(log, sizeof(log), "%s Failed to parse public key", tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);

        return val_ret_t::SIG_FAIL;
    }

    // Verify the signature hash against the firmware hash.
    int ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, firmwareHash, 32, 
        signature, signatureLen);
    
    mbedtls_pk_free(&pk); // Free the public key context

    if (ret == 0) {
        snprintf(log, sizeof(log), "%s Signature Valid", tag);
        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
            log, Messaging::Method::SRL_LOG);

        return val_ret_t::SIG_OK; // Signature is valid
    } else {
        snprintf(log, sizeof(log), "%s Signature invalid. Error Code %d", 
            tag, ret);

        Messaging::MsgLogHandler::get()->handle(Messaging::Levels::ERROR,
            log, Messaging::Method::SRL_LOG);

        return val_ret_t::SIG_FAIL; // Signature is invalid
    }
}

}