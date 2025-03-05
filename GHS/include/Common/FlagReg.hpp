#ifndef FLAGREG_HPP
#define FLAGREG_HPP

#include "cstdint"
#include "stddef.h"

namespace Flag {

class FlagReg {
    private:
    const char* tag;
    size_t reg; // Max size per system, either 32 or 64.

    public:
    FlagReg(const char* tag); 
    bool getFlag(uint8_t bitIdx);
    void setFlag(uint8_t bitIdx);
    void releaseFlag(uint8_t bitIdx);
};

}

#endif // FLAGREG_HPP