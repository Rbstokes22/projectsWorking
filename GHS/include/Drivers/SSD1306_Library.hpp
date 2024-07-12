#ifndef SSD1306_LIBRARY_H
#define SSD1306_LIBRARY_H

#include "config.hpp"
#include "Drivers/fonts.hpp"
#include "I2C/I2C.hpp"

namespace UI_DRVR {

enum class Size {
    OLEDbytes = 1024,
    pages = 8,
    columns = 128,
    bufferSize = 1088
};

enum class CMD_IDX {
    CMD_MODE, 
    SET_COL_ADDR, BEGIN_COL, END_COL,
    SET_PAGE_ADDR, BEGIN_PAGE, END_PAGE,
    DATA_MODE
};

enum class DIM {
    D5x7, D6x8
};

enum class TXTCMD {
    START, END
};

enum class OLED_ERR {
 // TO be completed
};

struct Dimensions {
    uint8_t width; 
    uint8_t height;
};

extern Dimensions dimIndex[];

class OLEDbasic {
    private:
    uint8_t col, page;
    Dimensions charDim;
    DIM dimID;
    size_t colMax, pageMax, cmdSeqLgth, lineLgth; 
    i2c_master_dev_handle_t i2cHandle;    
    static const uint8_t init_sequence[];
    static uint8_t charCMD[];

    uint8_t* Worker;
    uint8_t* Display;
    uint8_t bufferA[static_cast<int>(Size::bufferSize)];
    uint8_t bufferB[static_cast<int>(Size::bufferSize)];
    uint16_t bufferIDX;
    bool isBufferA;
    void grabChar(char c);
    void writeLine();
    
    public:
    OLEDbasic();
    bool init(uint8_t address);
    void makeTemplate();
    void reset(bool totalClear = false);
    void setCharDim(DIM dimension);
    void setPOS(uint8_t column, uint8_t page);
    void write(const char* msg, TXTCMD cmd = TXTCMD::END);
    void cleanWrite(const char* msg, TXTCMD cmd = TXTCMD::END);
    void send();
    size_t getOLEDCapacity() const;
};

}

#endif // SSD1306_LIBRARY_H