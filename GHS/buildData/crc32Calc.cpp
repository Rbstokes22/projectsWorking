#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <string>

// THIS IS USED TO GENERATE CHECKSUM VALUES FOR THE FIRMWARE.SIG AND ITS BACKUP
uint32_t computeCS(const uint8_t* data, size_t size) {

    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t crc32 = 0xFFFFFFFF;

    for(size_t addr = 0; addr < size; addr++) {
 
        crc32 ^= bytes[addr];

        for(size_t bit = 0; bit < 8; bit++) {
            if (crc32 & 1) {
                crc32 = (crc32 >> 1) ^ 0xEDB88320;
            } else {
                crc32 >>= 1;
            }
        }
    }

    return crc32 ^ 0xFFFFFFFF;
}

uint32_t ReadFile(const char* dir) {
   std::ifstream file(dir, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << dir << std::endl;
        return 0;
    }

    size_t out_size = file.tellg(); // Get the size of the file
    file.seekg(0, std::ios::beg); // Move back to the beginning of the file

    uint8_t* buffer = new uint8_t[out_size]; // Allocate buffer for the file contents

    file.read(reinterpret_cast<char*>(buffer), out_size); // Read the file into the buffer
    file.close(); // Close the file

    uint32_t CS = computeCS(buffer, out_size);
    delete buffer;
    return CS;
}

int main() {
    // INCLUDE absolute directories
    const char* dir = "/home/shadyside/Desktop/Programming/projects/GHS/data/app0firmware.sig";
    uint32_t CS = ReadFile(dir);
    std::cout.write(reinterpret_cast<const char*>(&CS), sizeof(CS));

    return 0;
}