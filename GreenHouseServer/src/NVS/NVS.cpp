#include "NVS/NVS.h"

// PUT CHECKSUM IN ITS OWN CLASS TO AVOID DUPLICATION MAYBE

namespace NVS {

// CORRESPONDS DIRECTLY WITH ENUM CLASS DATATYPE POSITION
const uint8_t dataSize[]{ 
    sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t),
    sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t), sizeof(uint64_t),
    sizeof(float_t), sizeof(double_t), sizeof(char)
};

// PRIVATE 
// This is how the error handling is handled throughout this program due 
// to the frequent error handling.
void NVSctrl::sendErr(const char* msg, bool endPref) {
    this->msglogerr.handle(
        Messaging::Levels::ERROR,
        msg,
        Messaging::Method::SRL
    );

    // Always calls to end prefs, unless otherwise indicated, which will happen
    // from methods that occur before prefs.begin() is called.
    if (endPref) this->prefs.end();
}

// Used in the checksum methods. Since their arguments are identical,
// it is modular to run the same check on variables.
bool NVSctrl::basicErrCheck(const char* key, const void* data, size_t size) {
    bool noError{true};

    if (data == nullptr || key == nullptr) {
        this->sendErr("NVS key or data set to nullptr");
        noError = false;
    }

    if (*key == '\0') {
        this->sendErr("NVS key does not exist");
        noError = false;
    }

    if (size <= 0) {
        this->sendErr("NVS size must be greater than 0");
        noError = false;
    }

    return noError; // returns true if no error
}

// PUBLIC
NVSctrl::NVSctrl(Messaging::MsgLogHandler &msglogerr, const char* nameSpace) :

msglogerr(msglogerr), nameSpace(nameSpace), isCheckSumSafe{true} {}

}