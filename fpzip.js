let nbind = require('nbind');
let lib = nbind.init().lib;

function _decompress(data, fn) {
  let zip = lib.Fpzip(data.buffer);
  let out = (zip.get_type() === 0) 
    ? (new Float32Array(zip.nvoxels()))
    : (new Float64Array(zip.nvoxels()));
  zip.decompress(data.buffer, out.buffer);
  return out;
}
 
exports.decompress = function (data) {
  return _decompress(data, 'decompress');
};

exports.dekempress = function (data) {
  return _decompress(data, 'dekempress');
};


