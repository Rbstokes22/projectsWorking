#include <iostream>

using async = void;
#define ASYNC(name) struct name

struct AddArgs {int a; int b;int* out;};

struct SubArgs {int a; int b; int c; int* out;};

void runFunc(void (*func)(void*), void* arg) {
    func(arg);
}

async add(void* arg) {
    AddArgs* args = static_cast<AddArgs*>(arg);
    *(args->out) = args->a + args->b;
}

async sub(void *arg) {
    SubArgs* args = static_cast<SubArgs*>(arg);
    *(args->out) = args->a - args->b - args->c;
}

int main() {

    int result = 0; 
    ASYNC(AddArgs) args {3, 4, &result};
 
    runFunc(add, &args);

    printf("Value is %d\n", result);

    int result2 = 0;

    ASYNC(SubArgs) args2 = {3, 4, 7, &result2};

    runFunc(sub, &args2);

    printf("Value is %d\n", result2);

    return 0;
}



