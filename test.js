var fs = require('fs');
var nbind = require('nbind');
var lib = nbind.init().lib;
 
function print(x) {
	console.log(x);
}

var filebytes = fs.readFileSync('./example.fpz');
var data = new Uint8Array(filebytes);

var zip = lib.Fpzip(data.buffer);

function decoded_data_buffer() {
	if (zip.get_type() == 0) {
		return new Float32Array(zip.nvoxels());
	}

	return new Float64Array(zip.nvoxels());
}

let out = decoded_data_buffer();
zip.decompress(data.buffer, out.buffer);

console.log(out.join(" "));

