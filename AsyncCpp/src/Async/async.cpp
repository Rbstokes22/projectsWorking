#include "Async/async.hpp"
#include <iostream>



namespace ASYNC {

async::async(int v) : val(v) {
    
    printf("ASYNC Created with value %d\n", v);
}

int async::getVal() {return this->val;}

}