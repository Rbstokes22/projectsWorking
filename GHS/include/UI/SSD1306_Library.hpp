#ifndef SSD1306_LIBRARY_H
#define SSD1306_LIBRARY_H

#include "config.hpp"
#include "fonts.hpp"
#include "I2C/I2C.hpp"

namespace UI_DRVR {

enum class Size {
    OLEDbytes = 1024,
    pages = 8,
    columns = 128
};

enum class CMD_IDX {
    CMD_MODE, 
    SET_COL_ADDR, BEGIN_COL, END_COL,
    SET_PAGE_ADDR, BEGIN_PAGE, END_PAGE
};

enum class DIM {
    D5x7, D6x8
};

enum class TXTCMD {
    START, END, STEND
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
    size_t colMax, pageMax; // non-static for future updates
    i2c_master_dev_handle_t i2cHandle;    
    static const uint8_t init_sequence[];
    static uint8_t charCMD[];
    static uint8_t clearBuffer1024[static_cast<int>(Size::OLEDbytes) + 1];
    void grabChar(char c, uint8_t* buffer, size_t idx, size_t bufferSize);
    void writeLine(uint8_t* buffer, size_t bufferSize);
    
    public:
    OLEDbasic();
    bool init(uint8_t address);
    void clear();
    void setCharDim(DIM dimension);
    void setPOS(uint8_t column, uint8_t page);
    void write(const char* msg, TXTCMD cmd = TXTCMD::STEND);
    void cleanWrite(const char* msg, TXTCMD cmd = TXTCMD::STEND);
};

}

#endif // SSD1306_LIBRARY_H