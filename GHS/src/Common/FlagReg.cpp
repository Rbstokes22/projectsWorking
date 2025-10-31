#include "Common/FlagReg.hpp"
#include "stdio.h"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace Flag {

// The register is size_t, meaning that you will only be able to use indicies
// from 0 to the bit-size of that object. Generally 32 or 64 flags. Recommend
// using enumeration to manage flag index positions
FlagReg::FlagReg(const char* tag) : tag(tag), reg(0) {

    char log[30]{0}; // Only used here to show object created.
    snprintf(log, sizeof(log), "%s Ob created", this->tag);
    Messaging::MsgLogHandler::get()->handle(Messaging::Levels::INFO,
        log, Messaging::Method::SRL_LOG, true); 
}

// Requires bitIndex of the flag. Returns true if set, and false if not.
bool FlagReg::getFlag(uint8_t bitIdx) {return ((this->reg >> bitIdx) & 0b1);}

// Require bitIndex of the flag. Sets the register bit to 1.
void FlagReg::setFlag(uint8_t bitIdx) {this->reg |= (0b1 << bitIdx);}

// Requires bitIndex of the flag. Sets the register bit to 0.
void FlagReg::releaseFlag(uint8_t bitIdx) {this->reg &= (~(0b1 << bitIdx));}

// Requires no params. Returns entire register reference.
size_t &FlagReg::getReg() {return this->reg;}

}