#ifndef FIRMWAREVAL_HPP
#define FIRMWAREVAL_HPP

#include "esp_partition.h"
#include "UI/MsgLogHandler.hpp"

namespace Boot {

#define PART_CHUNK_SIZE 1024 // bytes of chunks of partition data.
#define FW_FILEPATH_SIZE 35 // size of data used in filepath.

enum class val_ret_t { // valid return type
    SIG_OK, SIG_OK_BU, SIG_FAIL, PARTITION_OK,
    PARTITION_FAIL, VALID, INVALID
};

enum class PART { // Current or Next partition.
    CURRENT, NEXT
};

extern const char* pubKey; // public key generated from buildData.sh
extern bool CSsafe; // Checksum safe flag.
extern char log[LOG_MAX_ENTRY];
extern const char* tag;

val_ret_t checkPartition(PART type, size_t FWsize, size_t FWSigSize);
val_ret_t validateSig(const esp_partition_t* partition, size_t FWsize, 
    size_t FWSigSize);

val_ret_t readPartition(const esp_partition_t* partition, uint8_t* hash, 
    size_t FWsize);

size_t getSignature(uint8_t* signature, size_t sigSize, const char* label);
uint32_t computeCS(const uint8_t* data, size_t size);
val_ret_t verifySig(const uint8_t* firmwareHash, const uint8_t* signature, 
    size_t signatureLen);
}

#endif // FIRMWAREVAL_HPP 