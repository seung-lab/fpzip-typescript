let nbind = require('nbind');
let lib = nbind.init().lib;

function _decompress(data, fn) {
  let zip = lib.Fpzip(data.buffer);
  let out_buffer = (zip.get_type() === 0) 
    ? (new Float32Array(zip.nvoxels()))
    : (new Float64Array(zip.nvoxels()));
  zip[fn](data, out_buffer);
  return out_buffer;
}
 
function decompress(data) {
  return _decompress(data, 'decompress');
}

function dekempress(data) {
  return _decompress(data, 'dekempress');
}


