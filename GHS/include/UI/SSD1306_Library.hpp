#ifndef SSD1306_LIBRARY_H
#define SSD1306_LIBRARY_H

#include "config.hpp"
#include "fonts.hpp"
#include "I2C/I2C.hpp"

namespace UI {

enum class Size {
    OLEDbytes = 1024,
    charBuffer6x8 = 6,
    pages = 8,
    columns = 128,
    maxChars = 168
};

enum class CMD_IDX {
    CMD_MODE, 
    SET_COL_ADDR, BEGIN_COL, END_COL,
    SET_PAGE_ADDR, BEGIN_PAGE, END_PAGE
};

enum class OLED_ERR {

};

class OLEDbasic {
    private:
    uint8_t col, page;
    uint8_t charWidth, charHeight;
    size_t colMax, pageMax; // non-static for future updates
    i2c_master_dev_handle_t i2cHandle;    
    static uint8_t init_sequence[];
    static uint8_t charCMD[];
    static uint8_t clearBuffer1024[static_cast<int>(Size::OLEDbytes) + 1];
    static uint8_t charBuffer6x8[static_cast<int>(Size::charBuffer6x8) + 1];
    uint8_t dataSequence;
    void grab6x8(char c);
    void masterWrite(const char c);

    public:
    OLEDbasic();
    bool init(uint8_t address);
    void clear();
    void setCharDim(uint8_t width, uint8_t height);
    void write(const char* msg);
    void cleanWrite(const char* msg);
    void writeLn(const char* msg);


};

}















// void ssd1306_init(i2c_master_dev_handle_t &devHandle) {
    
//     // uint8_t init_sequence[] = {
//     //     0x00,           // Cmd mode
//     //     0xAE,           // Display OFF (sleep mode)
//     //     0x20, 0x00,     // Set Memory Addressing Mode to Horizontal Addressing Mode
//     //     0xB0,           // Set Page Start Address for Page Addressing Mode
//     //     0xC8,           // Set COM Output Scan Direction
//     //     0x00,           // Set lower column address
//     //     0x10,           // Set higher column address
//     //     0x40,           // Set display start line to 0
//     //     0x81, 0x7F,     // Set contrast control
//     //     0xA1,           // Set Segment Re-map
//     //     0xA6,           // Set display to normal mode (not inverted)
//     //     0xA8, 0x3F,     // Set multiplex ratio(1 to 64)
//     //     0xA4,           // Output follows RAM content
//     //     0xD3, 0x00,     // Set display offset to 0
//     //     0xD5, 0x80,     // Set display clock divide ratio/oscillator frequency
//     //     0xD9, 0xF1,     // Set pre-charge period
//     //     0xDA, 0x12,     // Set com pins hardware configuration
//     //     0xDB, 0x40,     // Set vcomh deselect level
//     //     0x8D, 0x14,     // Set Charge Pump
//     //     0xAF            // Display ON in normal mode
//     // };

//     // uint8_t clear_cmd[] = {
//     //     0x00, // command mode
//     //     0x21, 0x00, 0x08, // set column address of 0 - 127
//     //     0x22, 0x00, 0x07 // set page address from 0 - 7
//     // };

//     // uint8_t command_sequence[] = {
//     //     0x00,
//     //     0x21, 0x00, 0x05,
//     //     0x22, 0x00, 0x01
//     // };

//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, init_sequence, sizeof(init_sequence), -1));

//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, clear_cmd, sizeof(clear_cmd), -1));
//     // uint8_t clear_data[1025]{0}; // 128 x 64 / 8 = 1024 bytes to clear
//     // clear_data[0] = 0x40; // data mode
//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, clear_data, sizeof(clear_data), -1));


//     // uint8_t myval[7]{0};
//     // myval[0] = 0x40;
//     // memcpy(&myval[1], fonts + 18, 6);  

//     // i2c_master_transmit(devHandle, command_sequence, sizeof(command_sequence), -1);
//     // i2c_master_transmit(devHandle, myval, sizeof(myval), -1);
    





//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, command_sequence, sizeof(command_sequence), -1));
//     // // Prepare data to send, starting with data mode
//     // uint8_t data_sequence[9] = {0x40}; // First byte is data mode
//     // memcpy(&data_sequence[1], A_bitmap, sizeof(A_bitmap));

//     // // Send data sequence
//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, data_sequence, sizeof(data_sequence), -1));

//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, command_sequence2, sizeof(command_sequence2), -1));
//     // // Prepare data to send, starting with data mode
//     // uint8_t data_sequence2[14] = {0x40}; // First byte is data mode
//     // memcpy(&data_sequence2[1], transposed['K' - 32], sizeof(font_bitmap[0]));

//     // // Send data sequence
//     // ESP_ERROR_CHECK(i2c_master_transmit(devHandle, data_sequence2, sizeof(data_sequence2), -1));






// };







#endif // SSD1306_LIBRARY_H