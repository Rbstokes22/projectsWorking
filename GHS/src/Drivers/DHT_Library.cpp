#include "Drivers/DHT_Library.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Config/config.hpp"
#include "esp_timer.h"
#include "rom/ets_sys.h"

// Datasheet http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

namespace DHT_DRVR {

// Passes the pinval (LOW, HIGH) that is expected, as well as the timeout in 
// micros. Runs a loop while the level does not equal the expected pinval, 
// checking for a timeout. Once the value equals the expected value, it will
// break the loop, and compute the duration the pin is in the expected value.
// Returns 0 indicating timeout, or a unit32_t elapsed time in micros.
uint32_t DHT::getDuration(PVAL pinVal, uint16_t timeout_us) {
    uint16_t timeoutCt = 0;

    // Wait for state change before computing duration.
    while(gpio_get_level(this->pin) != static_cast<int>(pinVal)) {
        if (timeoutCt >= timeout_us) {
            return 0; // Indicates timeout.
        }

        timeoutCt++;
        ets_delay_us(1); // wait 1 micro to relax CPU.
    }

    // Measure the duration once the pin is in the correct mode.
    timeoutCt = 0; // Reset counter
    int64_t start = esp_timer_get_time(); // Reset start time

    while (gpio_get_level(this->pin) == static_cast<int>(pinVal)) {
        if (timeoutCt >= timeout_us) {
            return 0; // Indicates timeout
        }

        timeoutCt++;
        ets_delay_us(1); // wait 1 micro to relax CPU
    }

    return esp_timer_get_time() - start;
}

void DHT::calcF(float &temp) {
    temp = temp * 1.8f + 32.0f;
}

DHT::DHT(gpio_num_t pin) : pin(pin) {}

// Takes the temperature and humidity floats. Runs protocol to 
// signal the DHT to begin sending. Once signal is sent, 
// listens to the pinval to measure low and high durations.
// compiles duration times, and iterates them to determine the
// correct bit value, while writing that to the data to set
// temperature and humidity. Compares values against checksum
// and returns false if error, and true if data is good.
bool DHT::read(float &temp, float &hum, TEMP scale) {
    printf("Read\n");
    uint8_t data[5]{0};
    size_t bytesExp{5};
    uint32_t cycles[80]{0}; 

    // Set values to 0.
    temp = 0.0;
    hum = 0.0;

    // Set pin to input pullup and delay 1ms.
    // gpio_set_direction(this->pin, GPIO_MODE_INPUT); // PROBABLY CAN BE OMITTED, NOT CRITICAL IN DATASHEET
    // gpio_pullup_en(this->pin);
    // ets_delay_us(1000);

    // Send Signal to receive transmission
    gpio_set_direction(this->pin, GPIO_MODE_OUTPUT);
    gpio_set_level(this->pin, 0); // Pull pin low
    ets_delay_us(5000); // wait 5 ms datasheet says at least 1ms

    // Switch to input pullup mode to bring pin to high.
    gpio_set_direction(this->pin, GPIO_MODE_INPUT);
    gpio_pullup_en(this->pin);

    // Reply from the DHT is typically 20 - 40 micros

    // Waits for the pin to go low ~ 80 micros
    if (this->getDuration(PVAL::LOW, 1000) == 0) {
        printf("DHT timed out waiting for start signal low pulse\n");
        return false;
    }

    // Waits for pin to go high ~ 80 micros
    if (this->getDuration(PVAL::HIGH, 1000) == 0) {
        printf("DHT Timed out waiting for start signal high pulse\n");
        return false;
    }

    // Time critical operations. Uses the protocol from the DHT data sheet
    // to Check the duration of the low signal which should be ~ 50 micros,
    // followed by the duration of the high signal, which is ~26-28 micros
    // for a 0 bit, and ~70 micros for a 1 bit. This is separate to avoid 
    // as much lag as possible to ensure time criticality. 
    for (int i = 0; i < 80; i += 2) {
        cycles[i] = this->getDuration(PVAL::LOW, 100);
        cycles[i + 1] = this->getDuration(PVAL::HIGH, 100);
    }

    // Iterates each bit of each byte. References the index value of the 
    // cycles array, to check if the duration is > 40. If so, sets the 
    // appropriate bit to a 1 value.
    for (int byte = 0; byte < bytesExp; byte++) {
        data[byte] = 0; // init byte to 0

        for (int bit = 0; bit < 8; bit++) {
            uint8_t index = (byte * 16) + ((bit * 2) + 1);
            if (cycles[index] > 40) {
                data[byte] |= (1 << (7 - bit)); // set bit in correct position.
            }
        }
    }

    // Calculate humidity. Takes the 2 raw bytes and puts them into a 16 bit
    // value, and divides by 10.
    uint16_t humRaw = ((data[0] << 8) | data[1]);
    hum = humRaw / 10.0f;

    // Calculate temp. 0x7F handles the sign bit for negative temps by
    // stripping MS bit if value = 1. The sign is handled using 0x80
    // after combining data[2] and [3]. Works the same way as humidity.
    uint16_t tempRaw = (((data[2] & 0x7F) << 8) | data[3]);
    temp = tempRaw / 10.0f;
    if (data[2] & 0x80) temp = -temp; // negative temp handling

    // If the temp request is in F, converts to F.
    if (scale == TEMP::F) {
        this->calcF(temp);
    }
   
    // Calculate checksum. It is & with 0xFF because the values of the
    // data sums can exceed 255. In the event that it does, the checksum
    // will be the LSB of the value, so we want 8 values only.
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        printf("DHT Checksum Error\n");
        for (int i = 0; i < 80; i += 2) {
            printf("signal: %zu, value: %zu\n", (size_t)cycles[i], (size_t)cycles[i+1]);
        }
        return false;
    } else {
        return true; // data is good to use.
    }
}
    
}