
const myObj = function() {
    this.a = 22;
    this.b = 33;
    this.myFunc = myFunc;
    this.myFunc2 = myFunc2;
    return this;
}

let myFunc = function() {
    console.log(this.a);
}

let myFunc2 = () => {
    console.log(this.a);
}

let b = new myObj;
b.myFunc();
b.myFunc2(); // Undefines as expected due to arrow.

myFunc(); // Ret undefined as expected.
