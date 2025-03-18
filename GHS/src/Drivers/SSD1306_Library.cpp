#include "Drivers/SSD1306_Library.hpp"
#include <string.h>
#include <cstdint>
#include <cstddef>
#include "Config/config.hpp"
#include "Drivers/fonts.hpp"
#include "I2C/I2C.hpp"
#include "UI/MsgLogHandler.hpp"
#include "string.h"

namespace UI_DRVR {

// Used in conjunctions with the DIM enum to extract the dimensions.
Dimensions dimIndex[]{{5, 7}, {6, 8}};

// Static Setup

uint8_t const OLEDbasic::init_sequence[]{ // Set by datasheet and adafruit.
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
    0x40 // Data command
};

// The char is sent here and must be ASCII chars from 32 - 126.
void OLEDbasic::grabChar(char c) {
    if (c < 32 || c > 126) return; // omits chars beyond range.

    // The font arrays start at ASCII 32. The desired char subtracts
    // 32 from its value and its row is determined by the char width.
    uint16_t starting = (c - 32) * this->charDim.width; // Starting index.

    // Ensures no buffer overflow by determining current index plus the addit
    // character. Copies the bytes of that character to the Working buffer.
    if ((this->bufferIDX + this->charDim.width) <= 
        static_cast<int>(Size::bufferSize)) {

        // Ensures that the 
        if (this->dimID == DIM::D5x7) {
            memcpy(&Worker[this->bufferIDX], font5x7 + starting, 
                this->charDim.width);

        } else if (this->dimID == DIM::D6x8) {
            memcpy(&Worker[this->bufferIDX], font6x8 + starting, 
                this->charDim.width);
        }

        // Increases both the column and buffer index by the added char width.
        this->col += this->charDim.width;
        this->bufferIDX += this->charDim.width;

    } else {

        snprintf(this->log, sizeof(this->log), "%s buffer capacity full", 
            this->tag);

        this->sendErr(this->log);
    }
}

// Once the next character, or word goes beyond the capacity of the OLED,
// as well as the last line, a line will write to the Working buffer. This
// method adjusts the buffer index to the next open byte which will be offset
// by the length of the command sequence, from the first byte in that page/row.
// This prevent overwriting the command data within the template.
void OLEDbasic::writeLine() {
    this->col = 0; // Set column to 0
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

// Requires message, message level, and if repeating log analysis should be 
// ignored. Messaging default to ERROR, ignoreRepeat default to false.
void OLEDbasic::sendErr(const char* msg, Messaging::Levels lvl, 
    bool ignoreRepeat) {

    Messaging::MsgLogHandler::get()->handle(lvl, msg, SSD1306_LOG_METHOD, 
        ignoreRepeat);
}

// Defaults to 5x7 char font upon creation.
OLEDbasic::OLEDbasic() : 

    tag("(SSD1306)"),
    col{0x00}, page{0x00}, charDim{5, 7}, dimID{DIM::D5x7},
    colMax{static_cast<int>(Size::columns) - 1}, // index 7
    pageMax{static_cast<int>(Size::pages) - 1}, // index 127

    // Despite the exclusion of 0x40 data signal, it is captured exactly once.
    cmdSeqLgth(sizeof(OLEDbasic::charCMD) - 1), // excludes the 0x40
    lineLgth(static_cast<int>(Size::columns) + 1), // includes 0x40

    isInit(false),
    Worker{this->bufferA}, // sets worker pointer to buffer A
    Display{this->bufferB},
    bufferIDX{0}, isBufferA{true} {

        memset(this->log, 0, sizeof(this->log));
        memset(this->templateBuf, 0, sizeof(this->templateBuf));
        memset(this->bufferA, 0, sizeof(this->bufferA));
        memset(this->bufferB, 0, sizeof(this->bufferB));
        
        snprintf(this->log, sizeof(this->log), "%s Ob Created", this->tag);
        this->sendErr(this->log, Messaging::Levels::INFO, true);
    }

// Initializes at def i2c freq. Configures the i2c settings, and adds the i2c 
// device to the bus, receiving the handle in return.
bool OLEDbasic::init(uint8_t address) {
    if (this->isInit) return true; // Prevents from re-init.

    // Init the i2c and add device to the register.
    Serial::I2C* i2c = Serial::I2C::get();
    i2c_device_config_t devCon = i2c->configDev(address);
    this->i2cHandle = i2c->addDev(devCon);

    esp_err_t err = i2c_master_transmit( // Transmit init sequence
        this->i2cHandle, this->init_sequence,
        sizeof(this->init_sequence), SSD1306_I2C_TIMEOUT);

    this->isInit = true;
    this->reset(true); // Reset, clear, and make template.

    return (err == ESP_OK);
}

// Requires no params. Upon init, populates class obect template, that serves
// as a template buffer that is copied over to the worker buffer on all 
// subsequential calls. 
void OLEDbasic::makeTemplate() {
    static bool isInit = false; // Allows template to be created once to copy.

    // Sequence is written on the interval of the command sequence + the line
    // length, or 136 total. The template is pre-populated to include the 
    // command sequences within certain intervals of the template.

    // Once init, continutes copying the template buffer over to the worker.
    if (isInit) {
        memcpy(this->Worker, this->templateBuf, sizeof(this->templateBuf));
        return;
    }

    // ACTUAL TEMPLATE FORMAT, ONLY CMDSEQ ARE POPULATED WITHIN THE TEMPLATE.
    // R1: <BYTE0-CMDSEQ-BYTE7><BYTE8-CHARS-BYTE135>
    // R2: <BYTE136-CMDSEQ-BYTE143><BYTE144-CHARS-BYTE272>
    // ...
    // R8: <BYTE952-CMD_SEQ-BYTE959><BYTE960-CHARS-BYTE1088>

    // Everything below is for the initial creation of the template. 
    uint16_t i = this->lineLgth + this->cmdSeqLgth;
    static const uint8_t col{0};
    static const uint8_t endCol{static_cast<int>(Size::columns) - 1}; // 127
    uint8_t page{0};

    // Copies the command sequence into the Worker buffer starting
    // at index 0. Increments the page and allows the iteration to
    // write the rest. The columns will always remain the same. Places
    // first command sequence at index 0 - 7.
    memset(this->templateBuf, 0, static_cast<int>(Size::bufferSize)); // clear 
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = col;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = endCol;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = page;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = page + 1; // nx pg

    // copy command sequence into the working buffer.
    memcpy(this->templateBuf, OLEDbasic::charCMD, sizeof(OLEDbasic::charCMD));
    page++;
    
    // Iterates the remaining buffer copying the command sequences with
    // the adjusted pages into it. Increments i by the change in the 
    // line and command sequence, which will be 136. This places command 
    // sequences on the beginning of each page, that handles the chars in the
    // remaining portion of the line. Essentially this template pre-positions
    // the commands where all changes are expected to occur.
    while (i < static_cast<int>(Size::bufferSize)) {

        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = col;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = endCol;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = page;
        OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = page + 1;
        memcpy(&templateBuf[i], OLEDbasic::charCMD, sizeof(OLEDbasic::charCMD));
        i += (this->lineLgth + this->cmdSeqLgth); // Adjust i based on sizes
        page++; // Increment page by 1.
    }

    // Copy template over to worker.
    memcpy(this->Worker, this->templateBuf, sizeof(this->templateBuf));
    isInit = true; // Change to true to ensure template is created once.
}

// Resets the working buffer creating a new template, and 
// resetting the indexing data.
void OLEDbasic::reset(bool clearScreen) {
    makeTemplate(); // sets template for the working buffer.
    this->bufferIDX = sizeof(OLEDbasic::charCMD); // starts at end of cmdseq.
    this->col = 0;
    this->page = 0;

    // invoked upon init ONLY to clear the screen.
    if (clearScreen) this->send();
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
    this->col = column > this->colMax ? this->colMax : column;
    this->page = page > this->pageMax ? this->pageMax : page;

    // When the position is adjusted, computes the column and page 
    // adjustments to allow the buffer index to position itself 
    // correctly.
    uint16_t colAdjustment = sizeof(OLEDbasic::charCMD) +  this->col;
    uint16_t pageAdjustment = (this->cmdSeqLgth + this->lineLgth) * page;

    this->bufferIDX = colAdjustment + pageAdjustment;
}

// No text wrapping functionality. 
// TXTCMD::END is the default selection. This will write the text
// to the display and increment the page inserting a break.
// TXTCMD::START will ensure that you can display multiple pieces
// of text on the same line without incremementing the page.
void OLEDbasic::write(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {

        snprintf(this->log, sizeof(this->log), "%s No write msg", this->tag);
        this->sendErr(this->log);
        return;
    }

    uint8_t msgLen = strlen(msg);

    for (int i = 0; i < msgLen; i++) {

        // Omits a space from being first char in line.
        if(msg[i] == ' ' && this->col == 0) {
            continue;
        }

        this->grabChar(msg[i]); // writes char bytes to Worker buffer.

        // If the next char will extend beyond the column max, the 
        // line is written.
        if ((this->col + this->charDim.width) >= this->colMax) {
            this->writeLine(); // Adjusts buffer index
        }
    }

    // If col > 0, implies that text has been written to line. Iff END is 
    // called, will incremement page causing a line break. If END is not called
    // the buffer index will not be adjusted.
    if ((cmd == TXTCMD::END) && this->col > 0) {
        this->writeLine(); // Adjusts buffer index
    }
}

// Text wrapping functionality. 
// TXTCMD::END is the default selection. This will write the text
// to the display and increment the page, inserting a break.
// TXTCMD::START will ensure that you can display multiple pieces
// of text on the same line without incremementing the page.
void OLEDbasic::cleanWrite(const char* msg, TXTCMD cmd) {
    if (msg == nullptr || *msg == '\0') {

        snprintf(this->log, sizeof(this->log), "%s No cleanwrite msg", 
            this->tag);

        this->sendErr(this->log);
        return;
    }

    uint8_t msgLen = strlen(msg);
    uint8_t nextSpace{0};
    uint8_t i{0};

    while (i < msgLen) { // Iterate the length of the message.
        nextSpace = i; // Set equal to iterator value.

        // Iterates until it encounters a space, or the end of the message.
        // Incremenets nextSpace for each iteration. This will put the next
        // space at a different value than i, which will be the width that
        // the word requires.
        while(nextSpace < msgLen && msg[nextSpace] != ' ') {
            nextSpace++;
        }

        uint8_t wordLen = nextSpace - i; // Word length in chars.

        // Computes the number of pixels/columns that the next word will
        // occupy based on the word length.
        uint8_t pixels = (wordLen) * this->charDim.width;

        // If the single word extends beyond the column max, it will
        // be written with no text wrapping. It will then skip the 
        // rest of the iteration, and continue wrapping text.
        if (pixels >= static_cast<int>(Size::columns)) {
            char temp[210]{0}; // 204 is the max chars @ 5x7. Padded.
            
            snprintf(temp, wordLen, "%s", &msg[i]);
            
            // Sends START to preserve buffer index and avoid line break.
            this->write(temp, TXTCMD::START); 
            i += strlen(temp);
            continue;
        }
        
        // If the current column pos plus the next word length
        // exceeds the maximum value, it will write the current
        // line and begin the rest of everything on a new page
        // wrapping the text.
        if ((this->col + pixels) >= this->colMax) {
            this->writeLine(); // Adjusts buffer index
        } 

        // Iterates each word populating the line buffer.
        while (i <= nextSpace) {
            this->grabChar(msg[i]); 
            i++;
        }        
    }

    // Only writes if end is specified in the text command.
    if (cmd == TXTCMD::END && this->col > 0) {
        this->writeLine(); // Adjusts buffer index
    }
}

// Sends the Worker buffer via i2c to the OLED display.
void OLEDbasic::send() {
    esp_err_t err;

    auto errHandle = [this](esp_err_t err) { // Prints to serial.
        if (err != ESP_OK) {

            snprintf(this->log, sizeof(this->log), "%s send err. %s", 
                this->tag, esp_err_to_name(err));

            this->sendErr(this->log);
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

    reset(); // resets and creates a new Worker template. No screen clear.

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
                this->cmdSeqLgth, SSD1306_I2C_TIMEOUT
            );

            errHandle(err);

            toggle = false;
            i += this->cmdSeqLgth;
            
        } else {
            err = i2c_master_transmit(
                this->i2cHandle, &Display[i],
                this->lineLgth, SSD1306_I2C_TIMEOUT
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













