#ifndef FIRMWAREVAL_HPP
#define FIRMWAREVAL_HPP

#include "esp_partition.h"

namespace Boot {

enum class VAL {
    SIG_OK, SIG_OK_BU, SIG_FAIL, PARTITION_OK,
    PARTITION_FAIL, VALID, INVALID
};

enum class PART {
    CURRENT, NEXT
};

extern const char* pubKey;
extern bool CSsafe;

VAL checkPartition(PART type, size_t FWsize, size_t FWSigSize);

VAL validateSig(
    const esp_partition_t* partition, 
    size_t FWsize, 
    size_t FWSigSize
    );

VAL readPartition(
    const esp_partition_t* partition, 
    uint8_t* hash, 
    size_t FWsize
    );

size_t getSignature(
    uint8_t* signature, 
    size_t sigSize, 
    const char* label
    );

uint32_t computeCS(const uint8_t* data, size_t size);

VAL verifySig(
    const uint8_t* firmwareHash, 
    const uint8_t* signature, 
    size_t signatureLen
    );
}

#endif // FIRMWAREVAL_HPP 