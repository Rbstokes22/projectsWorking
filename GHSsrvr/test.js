
let id = 250;

const getID = () => {
    const _id = id;
    id++;
    id %= 256;
    
    return _id;
}

for (let i = 250; i < 260; i++) {
    console.log(getID());
}