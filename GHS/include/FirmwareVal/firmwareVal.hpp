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

enum class PART { // CURRENT or NEXT partition.
    CURRENT, NEXT
};

class FWVal {
    private:
    static const char* pubKey;
    const char* tag;
    char log[LOG_MAX_ENTRY];
    FWVal(); 
    FWVal(const FWVal&) = delete; // prevent copying
    FWVal &operator=(const FWVal&) = delete; // prevent assgnmt
    val_ret_t validateSig(const esp_partition_t* partition, size_t FWsize, 
        size_t FWSigSize);

    val_ret_t readPartition(const esp_partition_t* partition, uint8_t* hash, 
        size_t FWsize);

    size_t getSignature(uint8_t* signature, size_t sigSize, const char* label);
    val_ret_t verifySig(const uint8_t* firmwareHash, const uint8_t* signature, 
        size_t signatureLen);
    
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::WARNING);

    public:
    static FWVal* get();
    val_ret_t checkPartition(PART type, size_t FWsize, size_t FWSigSize);
};

}

#endif // FIRMWAREVAL_HPP 