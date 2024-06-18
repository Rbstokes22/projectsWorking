#include "Peripherals/PeripheralMain.h"

namespace FlashWrite {


    
}





namespace Peripheral {

void Sensors::lock() {
    this->mutex.lock();
}

void Sensors::unlock() {
    this->mutex.unlock();
}

}