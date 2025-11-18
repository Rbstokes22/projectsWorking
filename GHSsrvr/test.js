
let n = "http://greenhouse2.local";

let re = /^http:\/\/(greenhouse\d+)\.local$/;

console.log(re.exec(n)[1]);
