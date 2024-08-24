#ifndef FIRMWAREVAL_HPP
#define FIRMWAREVAL_HPP

#include "esp_partition.h"

namespace Boot {

enum class VAL {
    SIG_OK, SIG_OK_BU, SIG_FAIL, PARTITION_OK,
    PARTITION_FAIL
};

extern const size_t FIRMWARE_SIZE;
extern const char* pubKey;
extern bool CSsafe;

struct Files {
    const char* dir;
    const char* CS;
};

uint32_t computeCS(const uint8_t* data, size_t size);
size_t getSignature(uint8_t* signature, size_t size);
VAL readPartition(const esp_partition_t* partition, uint8_t* hash);
VAL verifySig(const uint8_t* firmwareHash, const uint8_t* signature, size_t signatureLen);
VAL validateSig(const esp_partition_t* partition);
void checkPartition();

}

#endif // FIRMWAREVAL_HPP 