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

// Static setup

// This is generated using buildData/buildKeys.sh. The public key is 
// copied into here.
const char* FWVal::pubKey =
    R"rawliteral(-----BEGIN PUBLIC KEY-----
    MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA8D+FXkYt9VnNbFXYJ1zn
    0wi4ARgpA7n2aDrJxj44pK77zyPEjkYYrQ2d1uHegHaag4g4NGsGw5HxIo54Uwb7
    q4T0i0J42FFD34sjwGNOqF56gQiTh5ELrVmuVpYCQOIwUIV5Hs/z+G2pKFVdPpmH
    DPj81fYnNsd5XLCK1ClWqf5NpPlSm6r8Y6tpU30pjQErkychV6o6HlgWsU3Ut9x5
    BGy5YrsqVUImN6uct4wMiPewYv1PaSVk3ABOVxQ33barx5rJtOfoyPfvz/zG8348
    gKIl1ssMaWQvgrJtnDoZWL+1jyatol0pnX9YrY0crbyGWSkDHH+PfOBkoiukVad5
    5wIDAQAB
    -----END PUBLIC KEY-----)rawliteral";

FWVal::FWVal() : tag("(FWVal)") {

    memset(this->log, 0, sizeof(this->log));
    snprintf(this->log, sizeof(this->log), "%s Ob created", this->tag);
    this->sendErr(this->log, Messaging::Levels::INFO);
}

// Requires the partition, firmware size, and the signature size. Reads the 
// partition to generate a hash. Gets the firmware signature, and verifies the
// hash against the signature hash. Returns SIG_OK or SIG_FAIL.
val_ret_t FWVal::validateSig(const esp_partition_t* partition, size_t FWsize, 
    size_t FWSigSize) {

    uint8_t hash[32]{0}; // hash will always be 32 bytes.
    uint8_t sig[FWSigSize]{0};

    char label[20]{0}; // Gets label name either app0 or app1
    strncpy(label, partition->label, sizeof(label)); // No chk needed

    if (partition == NULL) { // first check, is partition real.

        snprintf(this->log, sizeof(this->log), 
            "%s Failed to allocate memory for partition", this->tag);

        this->sendErr(this->log);

        return val_ret_t::SIG_FAIL; // Block remaining.
    }

    // If partition, Read the partition and generate the hash.
    if (this->readPartition(partition, hash, FWsize) == 
        val_ret_t::PARTITION_FAIL) {

        return val_ret_t::SIG_FAIL; // Logging in function call. Block.
    }

    // Get signature from spiffs.
    size_t sigLen = this->getSignature(sig, sizeof(sig), label);

    if (sigLen != FWSigSize) { // Compare it against the expected size.
        return val_ret_t::SIG_FAIL; // Logging in function call
    } 

    // If all is well, verify signature hash against firmware hash.
    return verifySig(hash, sig, sigLen);
}

// Requires the partition, hash, and firmware size. Reads the partition and
// generates a sha256 hash. Returns PARTITION_OK or PARTITION_FAIL.
val_ret_t FWVal::readPartition(const esp_partition_t* partition, uint8_t* hash, 
    size_t FWsize) {

    // Buffer used to read server response and write chunk data to flash.
    // Will be used over and over to compute the hash in the loop below.
    uint8_t buffer[PART_CHUNK_SIZE]{0};

    // Init the SHA-256 context
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);

    int sha256 = mbedtls_sha256_starts(&ctx, 0);

    if (sha256 < 0) { // Indicates error
        snprintf(this->log, sizeof(this->log),
            "%s mbedtls failed to start", this->tag); // Prep msg

        this->sendErr(this->log);
        return val_ret_t::PARTITION_FAIL;

    } else if (sha256 >= 0) { // Indicates success
        snprintf(this->log, sizeof(this->log),
            "%s mbedtls start success", this->tag); // Prep msg

        this->sendErr(this->log, Messaging::Levels::INFO);
    }

    // Iterates through the firmware partition in chunks. Updates the sha256 
    // hash as the iteration continues.
    for (size_t offset = 0; offset < FWsize; offset += PART_CHUNK_SIZE) {
        
        // Size left to read. If offset plus the chunk size exceeds the 
        // firmware size, toRead is set to the firmware size - the offset.
        // If fasle, to read will be the chunk size. This Ensures that the
        // remaining bytes to read are captured if smaller than the chunk size.
        size_t toRead = (offset + PART_CHUNK_SIZE > FWsize) ? 
            (FWsize - offset) : PART_CHUNK_SIZE;

        // Reads partition.
        esp_err_t err = esp_partition_read(partition, offset, buffer, toRead);

        if (err != ESP_OK) { // partition read error.

            snprintf(this->log, sizeof(this->log), 
                "%s Failed to read partition. %s", this->tag,
                esp_err_to_name(err));

            this->sendErr(this->log);

            mbedtls_sha256_free(&ctx); // If err, free sha256 context.
            return val_ret_t::PARTITION_FAIL; // Exit
        }

        // Feeds buffer into running sha256 computation if read properly.
        if (mbedtls_sha256_update(&ctx, buffer, toRead) != 0) { // 0 = success

            snprintf(this->log, sizeof(this->log), "%s Err updating sha256", 
                this->tag);

            this->sendErr(this->log);

            mbedtls_sha256_free(&ctx); // If err, free sha256 context.
            return val_ret_t::PARTITION_FAIL; // Exit
        }
    }

    sha256 = mbedtls_sha256_finish(&ctx, hash); // Finish the hash.

    if (sha256 < 0) { // Indicates error.

        snprintf(this->log, sizeof(this->log), "%s failed to finish sha256", 
            this->tag); 

        this->sendErr(this->log);
        mbedtls_sha256_free(&ctx); // Free sha256 if err.
        return val_ret_t::PARTITION_FAIL;
    } 

    // No error, continue on.
    snprintf(this->log, sizeof(this->log), "%s sha256 hash complete", 
        this->tag); // Prep message for OK.

    this->sendErr(this->log, Messaging::Levels::INFO);
    mbedtls_sha256_free(&ctx); // Free sha256 context when done.
    return val_ret_t::PARTITION_OK;
}

// Requires point to signature, the signature size, and the partition label.
// Copies the spiffs signature to the passed pointer, and returns the size
// of the signature.
size_t FWVal::getSignature(uint8_t* signature, size_t sigSize, 
    const char* label) {

    size_t bytesRead{0};
    uint8_t buffer[sigSize]; 
    char filepath[FW_FILEPATH_SIZE]{0};

    // Copy the path into filepath. This is how the spiffs storage is designed.
    snprintf(filepath, sizeof(filepath), "/spiffs/%sfirmware.sig", label);

    // Log activity
    snprintf(this->log, sizeof(this->log), "%s reading sig filepath: %s", 
        this->tag, filepath);

    this->sendErr(this->log, Messaging::Levels::INFO);

    FILE* f = fopen(filepath, "rb"); // Opens with read bytes

    if (f == NULL) { // If null return, log

        snprintf(this->log, sizeof(this->log), "%s Unable to open file %s", 
            this->tag, filepath);

        this->sendErr(this->log);

    } else { // File is open.
        
        bytesRead = fread(buffer, 1, sizeof(buffer), f); // Read to buffer
        
        // Copies the signature portion of the buffer to the signature, and
        // the checksum portion to the storedCS value.
        memcpy(signature, buffer, sigSize); // 256 bytes for the signature
    }

    fclose(f); // Close file when done.

    return bytesRead; // Signature length, will be 0 if not read.
}

// Requires pointers to firmware hash array and signature, and signature 
// length. 
val_ret_t FWVal::verifySig(const uint8_t* firmwareHash, const uint8_t* signature, 
    size_t signatureLen) {

    mbedtls_pk_context pk; // public key container
    mbedtls_pk_init(&pk);

    // Load the public key stored as a static char* from this class. Parse
    // the key. If error, log.
    if (mbedtls_pk_parse_public_key(&pk, (const unsigned char*)FWVal::pubKey, 
        strlen(FWVal::pubKey) + 1) != 0) {

        snprintf(this->log, sizeof(this->log), 
            "%s Failed to parse public key", this->tag);

        this->sendErr(this->log);
        return val_ret_t::SIG_FAIL;
    }

    // Verify the signature hash against the firmware hash.
    int ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, firmwareHash, 32, 
        signature, signatureLen);
    
    mbedtls_pk_free(&pk); // Free the public key context

    if (ret == 0) { // Success.

        snprintf(this->log, sizeof(this->log), "%s Signature Valid", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO);

        return val_ret_t::SIG_OK; // Signature is valid

    } else { // Failure

        snprintf(this->log, sizeof(this->log), 
            "%s Signature Invalid. Err Code: %d", this->tag, ret);

        this->sendErr(this->log);
        return val_ret_t::SIG_FAIL; // Signature is invalid
    }
}

// Requires messagine and messaging level. Level is default to WARNING since 
// the program will continue running, but any issues with the firmware 
// validation will prevent it from being used in the future, and the last 
// stable version will be used.
void FWVal::sendErr(const char* msg, Messaging::Levels lvl) {
    Messaging::MsgLogHandler::get()->handle(lvl, msg,
        Messaging::Method::SRL_LOG);
}

FWVal* FWVal::get() {
    static FWVal instance;
    return &instance;
}

// Requires the partition type, size of firmware, and size of the firmware
// signature. Checks the partion and compares it against the signature of the
// firmware. Returns VALID or INVALID.
val_ret_t FWVal::checkPartition(PART type, size_t FWsize, size_t FWSigSize) {
    val_ret_t ret = val_ret_t::INVALID; // Default setting.

    snprintf(this->log, sizeof(log), 
        "%s Checking partition size %zu with a sig size %zu", this->tag, FWsize, 
        FWSigSize);

    this->sendErr(this->log, Messaging::Levels::INFO);

    // All error handling handled in callbacks.
    auto validate = [&ret, this, FWsize, FWSigSize](
        const esp_partition_t* part) {

        if (part != NULL && part != nullptr) { // Good partition, continute.

            snprintf(this->log, sizeof(this->log), 
                "%s Validating partition label: %s", this->tag, part->label);

            this->sendErr(this->log, Messaging::Levels::INFO);

            // Validate the signature. If good, set return to VALID.
            if (this->validateSig(part, FWsize, FWSigSize) == 
                val_ret_t::SIG_OK) {

                ret = val_ret_t::VALID;
            } 

        } else { // If error, return type is already set to INVALID.

            snprintf(this->log, sizeof(this->log), 
                "%s Partion label: %s = NULL", this->tag, part->label);

           this->sendErr(this->log);
        }
    }; 

    if (type == PART::CURRENT) { // Get current partition information
        validate(esp_ota_get_running_partition());

    } else if (type == PART::NEXT) { // get next partition information
        validate(esp_ota_get_next_update_partition(NULL));
        
    } else { // Partition type incorrect.

        snprintf(this->log, sizeof(this->log), 
            "%s Only partition CURRENT or NEXT", this->tag);

        this->sendErr(this->log);
    }

    // Depending on current or next partition, validate will only run once
    // and set the return to the appropriate setting.
    return ret;
}

}