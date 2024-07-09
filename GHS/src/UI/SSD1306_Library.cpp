#include "UI/SSD1306_Library.hpp"
#include <string.h>

namespace UI_DRVR {

// Used in conjunctions with the DIM enum to extract the dimensions.
Dimensions dimIndex[]{{5, 7}, {6, 8}};

// Static Setup

uint8_t const OLEDbasic::init_sequence[]{
    0x00,           // Cmd mode
    0xAE,           // Display OFF (sleep mode)
    0x20, 0x00,     // Set Memory Addressing Mode to Horizontal Addressing Mode
    0xB0,           // Set Page Start Address for Page Addressing Mode
    0xC8,           // Set COM Output Scan Direction
    0x00,           // Set lower column address
    0x10,           // Set higher column address
    0x40,           // Set display start line to 0
    0x81, 0x7F,     // Set contrast control
    0xA1,           // Set Segment Re-map
    0xA6,           // Set display to normal mode (not inverted)
    0xA8, 0x3F,     // Set multiplex ratio(1 to 64)
    0xA4,           // Output follows RAM content
    0xD3, 0x00,     // Set display offset to 0
    0xD5, 0x80,     // Set display clock divide ratio/oscillator frequency
    0xD9, 0xF1,     // Set pre-charge period
    0xDA, 0x12,     // Set com pins hardware configuration
    0xDB, 0x40,     // Set vcomh deselect level
    0x8D, 0x14,     // Set Charge Pump
    0xAF            // Display ON in normal mode
};

// This can stay static. The only modifications to this will be the the
// addresses of the column and page, which is done before each command.
uint8_t OLEDbasic::charCMD[]{
    0x00, // Command mode
    0x21, 0x00, 0x7F, // Set column addr 0 - 127
    0x22, 0x00, 0x01 // Set page address 0 - 7
};

// All zeros in order to clear the buffer.
uint8_t OLEDbasic::clearBuffer1024[static_cast<int>(Size::OLEDbytes) + 1]{0}; // +1 include the data cmd

// The start of the font arrays are from ASCII 32 - 127, leaving you with 95 options. 
// the char is sent here and the starting position in computed. Depending on the 
// selected dimensions, the appropriate font array sequence fills the buffer and 
// adjusts the new column position.
void OLEDbasic::grabChar(char c, uint8_t* buffer, size_t idx, size_t bufferSize) {
    if (c < 32 || c > 126) return;

    uint16_t starting = (c - 32) * this->charDim.width;

    if (idx + this->charDim.width <= bufferSize) {
        if (this->dimID == DIM::D5x7) {
            memcpy(&buffer[idx], font5x7 + starting, 5);

        } else if (this->dimID == DIM::D6x8) {
            memcpy(&buffer[idx], font6x8 + starting, 6);
        }

        this->col += this->charDim.width;

    } else {
        // handle overflow in future
    }
}

// Defaults to 5x7 char font.
OLEDbasic::OLEDbasic() : 

    col{0x00}, page{0x00}, charDim{5, 7}, dimID{DIM::D5x7},
    colMax{static_cast<int>(Size::columns) - 1}, 
    pageMax{static_cast<int>(Size::pages) - 1} {}

// Initializes at 400khz. Configures the i2c settings, and adds the i2c 
// device to the bus, receiving the handle in return.
bool OLEDbasic::init(uint8_t address) {
    OLEDbasic::clearBuffer1024[0] = 0x40; // sets leading data cmd.
    I2C::i2c_master_init(I2C_DEF_FRQ);
    i2c_device_config_t devCon = I2C::configDev(address); 
    i2cHandle = I2C::addDev(devCon);

    ESP_ERROR_CHECK(i2c_master_transmit(
        this->i2cHandle, this->init_sequence,
        sizeof(this->init_sequence), -1));

    this->clear();

    return true; // change with errhandler
}

void OLEDbasic::clear() {

    // Spans the entire area of the OLED.
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = 0x00;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = 0x7F;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = 0x00;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = 0x07;
    this->col = 0;
    this->page = 0;

    ESP_ERROR_CHECK(i2c_master_transmit(
        this->i2cHandle, this->charCMD, 
        sizeof(this->charCMD), -1));

    ESP_ERROR_CHECK(i2c_master_transmit(
        this->i2cHandle, this->clearBuffer1024, 
        sizeof(this->clearBuffer1024), -1));
}

void OLEDbasic::setCharDim(DIM dimension) {
    dimID = dimension;
    Dimensions dims{dimIndex[static_cast<int>(dimension)]};
    this->charDim.width = dims.width;
    this->charDim.height = dims.height; 
}

void OLEDbasic::setPOS(uint8_t column, uint8_t page) {

    // Ensures everything is within parameters.
    if (column > this->colMax) this->col = this->colMax;
    if (page > this->pageMax) this->page = this->pageMax;

    this->col = column;
    this->page = page;
}

// This driver writes lines at a time, or a page. The amount of columns
// are taken into consideration, and once it runs out of space or is 
// complete, the line is written. The column is reset and page is inc.
void OLEDbasic::writeLine(uint8_t* buffer, size_t bufferSize) {
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = 0x00;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = 0x7F;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = this->page;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = this->page + 1;

    // Does nothing if exceeding the span of the OLED.
    if (this->page > (this->pageMax - 1)) return;

    i2c_master_transmit(
        this->i2cHandle, this->charCMD,
        sizeof(this->charCMD), -1);

    i2c_master_transmit(
        this->i2cHandle, buffer,
        bufferSize, -1);

    this->col = 0;
    this->page++;
}

// This populates the line buffer to send to the writeLine function.
void OLEDbasic::write(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
    }

    static uint8_t lineBuffer[static_cast<int>(Size::columns) + 1]{0};

    // Only upon a fresh start will the line buffer be reset. This prevent
    // page refresh when attemtping to write to the same page with different
    // invocations.
    if (cmd == TXTCMD::START || cmd == TXTCMD::STEND) {
        memset(lineBuffer, 0, sizeof(lineBuffer));
        lineBuffer[0] = 0x40;
    } 

    uint8_t msgLen = strlen(msg);

    for (int i = 0; i < msgLen; i++) {

        // Omits a space from being first char in line.
        if(msg[i] == ' ' && this->col == 0) {
            continue;
        }

        grabChar(msg[i], lineBuffer, 1 + (this->col), sizeof(lineBuffer));

        // Looks to see if the next char will exceed the line buffer. If it 
        // does it will write the line and clear the buffer, overriding the
        // specificity of END in the text cmd.
        if ((this->col + this->charDim.width) >= this->colMax) {
            this->writeLine(lineBuffer, sizeof(lineBuffer)); 
            memset(lineBuffer, 0, sizeof(lineBuffer)); 
            lineBuffer[0] = 0x40;
        }
    }

    // Only writes if end is specified in the text command.
    if ((cmd == TXTCMD::END || cmd == TXTCMD::STEND) && this->col > 0) {
        this->writeLine(lineBuffer, sizeof(lineBuffer));
    }
}

// Looks at the length of the next word, If it exceeds the line length,
// it will begin it on the next column.
void OLEDbasic::cleanWrite(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
    }

    static uint8_t lineBuffer[static_cast<int>(Size::columns) + 1]{0};

    // The same rules from the write function.
    if (cmd == TXTCMD::START || cmd == TXTCMD::STEND) {
        memset(lineBuffer, 0, sizeof(lineBuffer));
        lineBuffer[0] = 0x40;
    }

    uint8_t msgLen = strlen(msg);
    uint8_t nextSpace{0};
    uint8_t i{0};

    while (i < msgLen) {
        nextSpace = i;

        // Find and set the next space index value.
        while(nextSpace < msgLen && msg[nextSpace] != ' ') {
            nextSpace++;
        }

        // Computes the number of columns that the next word will
        // occumpy.
        uint8_t wordLen = (nextSpace - i) * this->charDim.width;

        // write without text wrapping if word exceeds limit. 
        if (wordLen >= static_cast<int>(Size::columns)) {
            this->clear();
            this->write(msg);
            return;
        }
        
        // If the current column pos plus the next word length
        // exceeds the maximum value, it will write the current
        // line and begin the rest of everything on a new page.
        if ((this->col + wordLen) >= this->colMax) {
            this->writeLine(lineBuffer, sizeof(lineBuffer)); 
            memset(lineBuffer, 0, sizeof(lineBuffer)); 
            lineBuffer[0] = 0x40; 
        } 

        // Iterates each word populating the line buffer.
        while (i <= nextSpace) {
            this->grabChar(msg[i], lineBuffer, 1 + this->col, sizeof(lineBuffer)); 
            i++;
        }        
    }

    // Only writes if end is specified in the text command.
    if ((cmd == TXTCMD::END || cmd == TXTCMD::STEND) && this->col > 0) {
        this->writeLine(lineBuffer, sizeof(lineBuffer));
    }
}

}

















