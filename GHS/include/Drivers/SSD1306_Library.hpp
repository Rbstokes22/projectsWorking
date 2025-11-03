#ifndef SSD1306_LIBRARY_H
#define SSD1306_LIBRARY_H

#include <cstdint>
#include <cstddef>
#include "I2C/I2C.hpp"
#include "UI/MsgLogHandler.hpp"
#include "Common/FlagReg.hpp"

// Datasheet
// https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf

namespace UI_DRVR {

#define SSD1306_I2C_TIMEOUT 100 // Timeout
#define SSD1306_LOG_METHOD Messaging::Method::SRL_LOG
#define SSD1306_TAG "(SSD_1306)"

enum class Size { // Size parameters, some specified by datasheet.
    pages = 8,
    columns = 128,
    bufferSize = 1088
};

enum class CMD_IDX { // Command Index ised with charCMD[].
    CMD_MODE, 
    SET_COL_ADDR, BEGIN_COL, END_COL,
    SET_PAGE_ADDR, BEGIN_PAGE, END_PAGE,
    DATA_MODE
};

enum class DIM { // Dimensions.
    D5x7, D6x8
};

// Text commands to start and end. Using START will ensure that you can 
// write multiple chars to the display without breaking the line. Using
// END will automatically increment to the next page once complete.
enum class TXTCMD {START, END};

// Used to prevent re-initialization.
enum class SSD_INIT : uint8_t {INIT, I2C_INIT, MK_TEMP};

struct Dimensions { // Will be used in conjunction with dimIndex[].
    uint8_t width; // Width dimension
    uint8_t height; // Height dimension
};

extern Dimensions dimIndex[];

class OLEDbasic {
    private:
    const char* tag;
    char log[LOG_MAX_ENTRY];
    Flag::FlagReg initFlag;
    uint8_t col, page; // Column and page values
    Dimensions charDim; // Width and height dimension struct
    DIM dimID; // Dimension ID, used with  enum class DIM.
    const size_t colMax, pageMax, cmdSeqLgth, lineLgth; // Max values.
    Serial::I2CPacket i2c; 
    static const uint8_t init_sequence[]; // Init sequence to start device
    static uint8_t charCMD[]; // character commands modified and reused.
    uint8_t* Worker; // Points to the worker buffer.
    uint8_t* Display; // Points to the display buffer.
    uint8_t templateBuf[static_cast<int>(Size::bufferSize)]; // Used as template
    uint8_t bufferA[static_cast<int>(Size::bufferSize)]; // Part 1 of dual buf.
    uint8_t bufferB[static_cast<int>(Size::bufferSize)]; // Part 2 of dual buf.
    uint16_t bufferIDX; // Current index of the next buffer entry.
    bool isBufferA; // Shows if buffer A or buffer B.
    void grabChar(char c);
    void writeLine();
    void sendErr(const char* msg, Messaging::Levels lvl = 
        Messaging::Levels::ERROR, bool ignoreRepeat = false);
    
    public:
    OLEDbasic();
    bool init(uint8_t address); 
    void makeTemplate();
    void reset(bool clearScreen = false);
    void setCharDim(DIM dimension);
    void setPOS(uint8_t column, uint8_t page);
    void write(const char* msg, TXTCMD cmd = TXTCMD::END);
    void cleanWrite(const char* msg, TXTCMD cmd = TXTCMD::END);
    void send();
    size_t getOLEDCapacity() const;
};

}

#endif // SSD1306_LIBRARY_H