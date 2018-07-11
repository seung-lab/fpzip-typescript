var fs = require('fs');
var nbind = require('nbind');
var lib = nbind.init().lib;
 
var filebytes = fs.readFileSync('./example.fpz');
var data = new Uint8Array(filebytes);

var zip = lib.Fpzip(data.buffer);