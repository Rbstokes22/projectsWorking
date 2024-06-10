#include "Peripherals.h"

DHT dht(DHT_PIN, DHT22);
Adafruit_AS7341 as7341;
// TaskHandle_t taskHandle = NULL; // This is just the entry, or handle to the thread

namespace Threads {

SensorThread::SensorThread(Clock::Timer &checkSensors) : 
    taskHandle(NULL), 
    checkSensors(checkSensors),
    isThreadSuspended(false) {}

void SensorThread::setupThread() {
    xTaskCreate(
        sensorTask, // function to be tasked by freeRTOS
        "Sensor Task", // Name used for debugging
        2048, // allocated stack size
        &this->checkSensors, // parameters to send, use struct if more than 1
        1, // priority
        &this->taskHandle // pointer to the task handle
    );
}

void SensorThread::sensorTask(void* parameter) {
    Clock::Timer* checkSensors = (Clock::Timer*) parameter;
    while (true) {
        if (checkSensors->isReady()) {
            handleSensors();
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // delays for 100 ms to prevent constant run
    }
}

void SensorThread::suspendTask() { // Called by OTA updates
    Serial.println("Task suspended");
    vTaskSuspend(taskHandle);
}

void SensorThread::resumeTask() {
    Serial.println("Task resumed");
    vTaskResume(taskHandle);
}
}

void handleSensors() {
    as7341.readAllChannels();
    Serial.println("================================");
    Serial.print("Photo: "); Serial.println(analogRead(Photoresistor_PIN));
    Serial.print("TEMP: "); Serial.println(dht.readTemperature());
    Serial.print("HUM: "); Serial.println(dht.readHumidity());
    Serial.print("Soil1: "); Serial.println((analogRead(Soil_1_PIN)));


    Serial.print("F1: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_415nm_F1));
    // Serial.print("F2: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_445nm_F2));
    // Serial.print("F3: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_480nm_F3));
    // Serial.print("F4: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_515nm_F4));
    // Serial.print("F5: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_555nm_F5));
    // Serial.print("F6: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_590nm_F6));
    // Serial.print("F7: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_630nm_F7));
    // Serial.print("F8: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_680nm_F8));
    // Serial.print("Clear: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_CLEAR));
    // Serial.print("NIR: "); Serial.println(as7341.getChannel(AS7341_CHANNEL_NIR));

}