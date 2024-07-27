#include "Drivers/SSD1306_Library.hpp"
#include <string.h>
#include <cstdint>
#include <cstddef>
#include "config.hpp"
#include "Drivers/fonts.hpp"
#include "I2C/I2C.hpp"

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

// Template for a basic command sent through i2c to set the column and 
// row space. The 0x40 is a data signal, and included in here due to
// repetition during template creation, but it is not sent with the
// command sequence.
uint8_t OLEDbasic::charCMD[]{
    0x00, // Command mode
    0x21, 0x00, 0x7F, // Set column addr 0 - 127
    0x22, 0x00, 0x01, // Set page address 0 - 7
    0x40 
};

// The char is sent here and must be ASCII chars from 32 - 126.
void OLEDbasic::grabChar(char c) {
    if (c < 32 || c > 126) return; // omits chars beyond range.

    // The font arrays start at ASCII 32. The desired char subtracts
    // 32 from its value and its row is determined by the char width.
    uint16_t starting = (c - 32) * this->charDim.width;

    // Ensures no buffer overflow by determining current index plus the additional
    // character. Copies the bytes of that character to the Working buffer.
    if (this->bufferIDX + this->charDim.width <= static_cast<int>(Size::bufferSize)) {
        if (this->dimID == DIM::D5x7) {
            memcpy(&Worker[this->bufferIDX], font5x7 + starting, this->charDim.width);

        } else if (this->dimID == DIM::D6x8) {
            memcpy(&Worker[this->bufferIDX], font6x8 + starting, this->charDim.width);
        }

        // Increases both the column and buffer index by the added char width.
        this->col += this->charDim.width;
        this->bufferIDX += this->charDim.width;

    } else {
        // handle overflow in future
    }
}

// Defaults to 5x7 char font upon creation.
OLEDbasic::OLEDbasic() : 

    col{0x00}, page{0x00}, charDim{5, 7}, dimID{DIM::D5x7},
    colMax{static_cast<int>(Size::columns) - 1}, // index 7
    pageMax{static_cast<int>(Size::pages) - 1}, // index 127
    cmdSeqLgth(sizeof(OLEDbasic::charCMD) - 1), // excludes the 0x40
    lineLgth(static_cast<int>(Size::columns) + 1), // includes 0x40
    Worker{this->bufferA}, // sets worker pointer to buffer A
    Display{this->bufferB},
    bufferIDX{0}, isBufferA{true} {

        memset(this->bufferA, 0, sizeof(this->bufferA));
        memset(this->bufferB, 0, sizeof(this->bufferB));
    }

// Initializes at 400khz. Configures the i2c settings, and adds the i2c 
// device to the bus, receiving the handle in return.
bool OLEDbasic::init(uint8_t address) {
    if (I2C::i2c_master_init(I2C_DEF_FRQ)) {
        i2c_device_config_t devCon = I2C::configDev(address); 
        this->i2cHandle = I2C::addDev(devCon);

        ESP_ERROR_CHECK(i2c_master_transmit(
            this->i2cHandle, this->init_sequence,
            sizeof(this->init_sequence), -1));

        this->reset(true); // sets Worker buffer to clear screen.

        return true; // change with errhandler
    } else {
        return false;
    }
}

// Creates a template to be written to. This sets the command sequence
// and the data signal. This sequence is written on the interval of 
// of the cmd sequence + line length or 136.
void OLEDbasic::makeTemplate() {
    uint16_t i = this->lineLgth + this->cmdSeqLgth;
    uint8_t col{0};
    uint8_t endCol{static_cast<int>(Size::columns) - 1}; // 127
    uint8_t page{0};

    // Copies the command sequence into the Worker buffer starting
    // at index 0. Increments the page and allows the iteration to
    // write the rest. The columns will always remain the same.
    memset(this->Worker, 0, static_cast<int>(Size::bufferSize));
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = col;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = endCol;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = page;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = page + 1;
    memcpy(this->Worker, OLEDbasic::charCMD, sizeof(OLEDbasic::charCMD));
    page++;
    
    while (i < static_cast<int>(Size::bufferSize)) {

        // Iterates the remaining buffer copying the command sequence with
        // the adjusted pages into it. Increments i by the change in the 
        // line and command sequence, which will be 136.
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = col;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = endCol;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = page;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = page + 1;
        memcpy(&Worker[i], OLEDbasic::charCMD, sizeof(OLEDbasic::charCMD));
        i += (this->lineLgth + this->cmdSeqLgth);
        page++;
    }
}

// Resets the working buffer creating a new template, and 
// resetting the indexing data.
void OLEDbasic::reset(bool totalClear) {
    makeTemplate(); // sets template for the working buffer.
    this->bufferIDX = sizeof(OLEDbasic::charCMD);
    this->col = 0;
    this->page = 0;

    // invoked upon init to clear the screen.
    if (totalClear) this->send();
}

// Dimensions must be set before positon due to indexing.
void OLEDbasic::setCharDim(DIM dimension) {
    dimID = dimension;
    Dimensions dims{dimIndex[static_cast<int>(dimension)]};
    this->charDim.width = dims.width;
    this->charDim.height = dims.height; 
}

// Dimensions must be set before position due to indexing.
// The POS for column will be between 0 & 127, and page will
// be between 0 & 7.
void OLEDbasic::setPOS(uint8_t column, uint8_t page) {

    // Ensures everything is within parameters.
    if (column > this->colMax) this->col = this->colMax;
    if (page > this->pageMax) this->page = this->pageMax;

    this->col = column;
    this->page = page;

    // When the position is adjusted, computes the column and page 
    // adjustments to allow the buffer index to position itself 
    // correctly.
    uint16_t colAdjustment = sizeof(OLEDbasic::charCMD) +  this->col;
    uint16_t pageAdjustment = (this->cmdSeqLgth + this->lineLgth) * page;

    this->bufferIDX = colAdjustment + pageAdjustment;
}

// Once the next character, or word goes beyond the capacity of the OLED,
// as well as the last line, a line will write to the Working buffer.
void OLEDbasic::writeLine() {
    this->col = 0;
    this->page++; // next page
    
    // Does nothing if exceeding the row span of the OLED.
    if (this->page > (this->pageMax)) return;

    // The buffer index is adjusted to the correct position based on 
    // its page and the length of bytes dedicated to the page (136).
    // Since a command sequence was written into the buffer at that 
    // index value, the buffer is incremented by the size of that.
    size_t bufferAdjustment = page * (this->cmdSeqLgth + this->lineLgth);

    this->bufferIDX = bufferAdjustment + sizeof(OLEDbasic::charCMD);
}

// No text wrapping functionality. 
// TXTCMD::END is the default selection. This will write the text
// to the display and increment the page inserting a break.
// TXTCMD::START will ensure that you can display multiple pieces
// of text on the same line without incremementing the page.
void OLEDbasic::write(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
    }

    uint8_t msgLen = strlen(msg);

    for (int i = 0; i < msgLen; i++) {

        // Omits a space from being first char in line.
        if(msg[i] == ' ' && this->col == 0) {
            continue;
        }

        grabChar(msg[i]); // writes char bytes to Worker buffer.

        // If the next char will extend beyond the column max, the 
        // line is written.
        if ((this->col + this->charDim.width) >= this->colMax) {
            this->writeLine(); 
        }
    }

    // Only writes if end is specified in the text command.
    if ((cmd == TXTCMD::END) && this->col > 0) {
        this->writeLine();
    }
}

// Text wrapping functionality. 
// TXTCMD::END is the default selection. This will write the text
// to the display and increment the page, inserting a break.
// TXTCMD::START will ensure that you can display multiple pieces
// of text on the same line without incremementing the page.
void OLEDbasic::cleanWrite(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
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
        // occupy.
        uint8_t wordLen = (nextSpace - i) * this->charDim.width;

        // If the single word extends beyond the column max, it will
        // be written with no text wrapping. It will then skip the 
        // rest of the iteration, and continue wrapping text.
        if (wordLen >= static_cast<int>(Size::columns)) {
            char temp[210]{0}; // 204 is the max chars @ 5x7.
            
            strncpy(temp, &msg[i], nextSpace - i);
            temp[nextSpace - i] = '\0';
        
            this->write(temp, TXTCMD::START); // start to no break line.
            i += strlen(temp);
            continue;
        }
        
        // If the current column pos plus the next word length
        // exceeds the maximum value, it will write the current
        // line and begin the rest of everything on a new page
        // wrapping the text.
        if ((this->col + wordLen) >= this->colMax) {
            this->writeLine(); 
        } 

        // Iterates each word populating the line buffer.
        while (i <= nextSpace) {
            this->grabChar(msg[i]); 
            i++;
        }        
    }

    // Only writes if end is specified in the text command.
    if (cmd == TXTCMD::END && this->col > 0) {
        this->writeLine();
    }
}

// Sends the Worker buffer via i2c to the OLED display.
void OLEDbasic::send() {
    esp_err_t err;

    auto errHandle = [](esp_err_t err) {
        if (err != ESP_OK) {
            printf("err: %s\n", esp_err_to_name(err));
        }
    };

    // Used to toggle between transmitting commands and data.
    bool toggle = true;

    // Toggles the display and worker pointers between bufferA
    // and bufferB. This is to allow dual buffers and improve 
    // the OLED display. This is better used with threads that 
    // can begin writing to the worker while sending the display.
    if (isBufferA) {
        this->Display = this->bufferA;
        this->Worker = this->bufferB;
        this->isBufferA = false;
    } else {
        this->Display = this->bufferB;
        this->Worker = this->bufferA;
        this->isBufferA = true;
    }

    reset(); // resets and creates a new Worker template.

    // TESTING. ALLOWS YOU TO SEE THE ACTUAL BYTES IN THE BUFFER
    // for (int i = 0; i < static_cast<int>(Size::bufferSize); i++) {
    //     printf("%d, ", Display[i]);
    // }

    uint16_t i{0};

    // I2C communications. Used with toggle to switch between
    // sending commands and data. The command sequence is 7 bytes
    // and the data is 129. This ensures perfect precision when
    // transmitting the 1088 byte ((129 + 7) * 8) buffer.
    while(i < static_cast<int>(Size::bufferSize)) {
        if (toggle) {
            err = i2c_master_transmit(
                this->i2cHandle, &Display[i],
                this->cmdSeqLgth, -1
            );

            errHandle(err);

            toggle = false;
            i += this->cmdSeqLgth;
        } else {
            err = i2c_master_transmit(
                this->i2cHandle, &Display[i],
                this->lineLgth, -1
            );

            errHandle(err);

            toggle = true;
            i += this->lineLgth;
        }
    }
}

// Returns character capacity based on the selected char size
size_t OLEDbasic::getOLEDCapacity() const {
    size_t totalBytes = (this->colMax + 1) * (this->pageMax + 1);
    return totalBytes / this->charDim.width;
}

}













