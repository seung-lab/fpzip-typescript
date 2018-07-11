var nbind = require('nbind');
var lib = nbind.init().lib;
 
var data = new Uint8Array([0,0,0,0,0,0,0,0,0,0]);

// console.log(lib);
// console.log(lib.Fpzip);

lib.Fpzip(data.buffer);