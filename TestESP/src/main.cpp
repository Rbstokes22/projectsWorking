#include "NVS2/NVS.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


extern "C" {
    void app_main();
}

NVS::NVSctrl nvs("tester");

struct Tester123 {
    int a;
    char b[15];
};

void app_main() {
    nvs.eraseAll();

    printf("Testing\n");
    vTaskDelay(pdMS_TO_TICKS(3000));

    Tester123 tt = {22286, "Hello"};

    printf("Writing struct tt\n");
    nvs.write("key1", &tt, sizeof(tt));
    vTaskDelay(pdMS_TO_TICKS(3000));

    Tester123 tt2 = {0, '\0'};

    printf("Reading strut\n");
    nvs.read("key1", &tt2, sizeof(tt2));

    printf("a = %d, b = %s\n", tt2.a, tt2.b);

    vTaskDelay(pdMS_TO_TICKS(3000));

}