#include "UI/SSD1306_Library.hpp"
#include <string.h>

namespace UI {

// Static Setup

uint8_t OLEDbasic::init_sequence[]{
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

uint8_t OLEDbasic::charCMD[]{
    0x00, // Command mode
    0x21, 0x00, 0x00, // Set column addr 0 - 127
    0x22, 0x00, 0x00 // Set page address 0 - 7
};

uint8_t OLEDbasic::clearBuffer1024[static_cast<int>(Size::OLEDbytes) + 1]{0}; // include the data cmd
uint8_t OLEDbasic::charBuffer6x8[static_cast<int>(Size::charBuffer6x8) + 1]{0};

void OLEDbasic::grab6x8(char c) {
    uint16_t starting = (c - 32) * 6;
    OLEDbasic::charBuffer6x8[0] = 0x40;
    memcpy(&OLEDbasic::charBuffer6x8[1], font6x8 + starting, 6);
}

void OLEDbasic::masterWrite(const char c) {
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_COL)] = this->col;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_COL)] = this->col + this->charWidth;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::BEGIN_PAGE)] = this->page;
    OLEDbasic::charCMD[static_cast<int>(CMD_IDX::END_PAGE)] = this->page + 0x01;


    if (this->charWidth == 6 && this->charHeight == 8) {
        i2c_master_transmit(
        this->i2cHandle, this->charCMD,
        sizeof(this->charCMD), -1);

        this->grab6x8(c);

        i2c_master_transmit(
        this->i2cHandle, this->charBuffer6x8,
        sizeof(this->charBuffer6x8), -1);

        this->col += 6;
    }
}

OLEDbasic::OLEDbasic() : 

    // default char dimensions 6 x 8
    col{0x00}, page{0x00}, charWidth{6}, charHeight{8},
    colMax{static_cast<int>(Size::columns)}, 
    pageMax{static_cast<int>(Size::pages)} {}

bool OLEDbasic::init(uint8_t address) {
    OLEDbasic::clearBuffer1024[0] = 0x40; // sets leading data cmd.
    I2C::i2c_master_init(400000);
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
    this->col = 0x00;
    this->page = 0x00;

    ESP_ERROR_CHECK(i2c_master_transmit(
        this->i2cHandle, this->charCMD, 
        sizeof(this->charCMD), -1));

    ESP_ERROR_CHECK(i2c_master_transmit(
        this->i2cHandle, this->clearBuffer1024, 
        sizeof(this->clearBuffer1024), -1));
}

void OLEDbasic::setCharDim(uint8_t width, uint8_t height) {
    this->charWidth = width;
    this->charHeight = height;
}

void OLEDbasic::write(const char* msg) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
    }

    uint8_t msgLen = strlen(msg);

    for (int i = 0; i < msgLen; i++) {
   
        // reset column and increment page.
        if ((this->col + this->charWidth) >= (this->colMax - 1)) {
            this->col = 0x00; this->page++;
        }

        if (this->page > (this->pageMax - 1)) {
            // Everything is full, do nothing, maybe put future alert here
            return;
        } 

        masterWrite(msg[i]);
    }
}

void OLEDbasic::cleanWrite(const char* msg) {
    if (msg == nullptr || *msg == '\0') {
        // handle print h
        return;
    }

    uint8_t nextSpace{0};
    uint8_t msgLen = strlen(msg);
    uint8_t i{0};

    while (i < msgLen) {
        nextSpace = i;

        while (nextSpace < msgLen && msg[nextSpace] != ' ') {
            nextSpace++;
        }

        uint8_t wordLen = nextSpace - i;
        uint8_t nextWordLen = this->charWidth * wordLen;

        if ((this->col + nextWordLen) >= this->colMax) {
            this->col = 0x00;
            this->page++;
        }

        if (this->page > (this->pageMax - 1)) {
            // handle in the future
            return;
        }

        masterWrite(msg[i++]);
    }
}

void OLEDbasic::writeLn(const char* msg) {
    // Write chunk maybe instead of writeLn? maybe just next page after
    // this write. Sort of like write line. Maybe a clean writeLN as well?
}
















}