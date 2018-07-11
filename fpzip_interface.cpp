/* fpzip_emscripten.cpp
 *
 * This is a C++ fpzip interface that can be compiled 
 * into Javascript asm for use with web apps. It supports
 * decompressing fpzip files and modified "kempressed" fpzip 
 * files that store only 0-1 values, have 2.0f added to them,
 * and have their axes permuted from XYZC to XYCZ.
 *
 * fpzip was originally authored by Peter Lindstrom et al at LLNL.
 *
 * Author: William Silversmith
 * Affiliation: Seung Lab, Princeton Neuroscience Institute
 * Date: July 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstring>

#include "nbind/api.h"
#include "nbind/nbind.h"

using namespace std;

#include "fpzip-1.2.0/inc/fpzip.h"
#include "fpzip-1.2.0/inc/read.h"

/* PROTOTYPES */

class Fpzip {
public:
  unsigned int type;
  unsigned int prec;
  size_t nx;
  size_t ny;
  size_t nz;
  size_t nf;

  Fpzip () {
    type = 0;
    prec = 0;
    nx = 0;
    ny = 0;
    nz = 0;
    nf = 0;
  }

  Fpzip (nbind::Buffer buf) {
    type = 0;
    prec = 0;
    nx = 0;
    ny = 0;
    nz = 0;
    nf = 0;
    decode_headers(buf);
  }

  Fpzip(Fpzip &orig) {
    type = orig.type;
    prec = orig.prec;
    nx = orig.nx;
    ny = orig.ny;
    nz = orig.nz;
    nf = orig.nf;
  }

  ~Fpzip() {
  
  }

  size_t nvoxels() {
    return nx * ny * nz * nf;
  }

  size_t nbytes() {
    // type: 0 = float, 1 = double
    return nvoxels() * (type + 1) * sizeof(float);
  }

  size_t get_type() { return type; }
  size_t get_prec() { return prec; }
  size_t get_nx() { return nx; }
  size_t get_ny() { return ny; }
  size_t get_nz() { return nz; }
  size_t get_nf() { return nf; }

  void decode_headers(nbind::Buffer &buffer) {
    unsigned char* data = buffer.data();

    char errorstr[128];

    FPZ* fpz = fpzip_read_from_buffer((void*)data);
    if (!fpzip_read_header(fpz)) {
      sprintf(errorstr, "cannot read header: %s\n", fpzip_errstr[fpzip_errno]);
      NBIND_ERR(errorstr);
      return;
    }
    type = fpz->type;
    prec = fpz->prec;
    nx = fpz->nx;
    ny = fpz->ny;
    nz = fpz->nz;
    nf = fpz->nf;
    
    fpzip_read_close(fpz);
  }

  void check_length(nbind::Buffer &buf) {
    char errorstr[512];

    if (buf.length() < nbytes()) {
      sprintf(errorstr, "Javascript TypedArray (%u bytes) should be at least %u bytes long.", 
        (unsigned int)buf.length(), (unsigned int)nbytes());
      NBIND_ERR(errorstr);
      return;
    }
  }

  void decompress(nbind::Buffer encoded, nbind::Buffer decoded) {
    decode_headers(encoded);
    check_length(decoded);
    dfpz(encoded, decoded);
    decoded.commit();
  }

  /* fpzip decompression + dekempression.
  *  
  * 1) fpzip decompress
  * 2) Subtract 2.0 from all elements.  
  * 3) XYCZ -> XYZC
  * 
  * Example:
  * DecodedImage *di = dekempress(buffer);
  * float* img = (float*)di->data;
  */
  void dekempress(nbind::Buffer encoded, nbind::Buffer decoded) {
    decode_headers(encoded);
    check_length(decoded);

    dfpz(encoded, decoded);

    unsigned char* decode_buffer = decoded.data();

    if (type == FPZIP_TYPE_FLOAT) {  
      dekempress_algo<float>( (float*)decode_buffer );
    }
    else {
      dekempress_algo<double>( (double*)decode_buffer );
    }

    decoded.commit();
  }

  /* Standard fpzip decompression. 
  * 
  * Example:
  * DecodedImage *di = decompress(buffer);
  * float* img = (float*)di->data;
  */
  void dfpz(nbind::Buffer encoded, nbind::Buffer decoded) {
    char errorstr[128];
    
    unsigned char* buffer = encoded.data();

    FPZ* fpz = fpzip_read_from_buffer((void*)buffer);

    if (!fpzip_read_header(fpz)) {
      sprintf(errorstr, "cannot read header: %s\n", fpzip_errstr[fpzip_errno]);
      NBIND_ERR(errorstr);
      return;
    }

    void* data = (void*)decoded.data();

    // perform actual decompression
    if (!fpzip_read(fpz, data)) {
      sprintf(errorstr, "decompression failed: %s\n", fpzip_errstr[fpzip_errno]);
      NBIND_ERR(errorstr);
      return;
    }

    fpzip_read_close(fpz);
  }

  template <typename T>
  void dekempress_algo(T *jsdata) {
    const size_t nvx = nvoxels();

    // Reverse loss of one bit by subtracting 2.0
    for (size_t i = 0; i < nvx; i++) {
      jsdata[i] -= 2.0;
    }

    // Change axes XYCZ to XYZC

    T *dekempressed = new T[nvx];
    T *src;
    T *dest;

    const size_t xysize = nx * ny;
    int offset = 0;

    for (size_t channel = 0; channel < nf; channel++) {
      offset = nx * ny * nz * channel;

      for (size_t z = 0; z < nz; z++) {
        src = &jsdata[ z * xysize * (nf + channel) ];
        dest = &dekempressed[ z * xysize + offset ];
        memcpy(dest, src, xysize * sizeof(T)); 
      }
    }

    memcpy(jsdata, dekempressed, nvx * sizeof(T));
  }
};

NBIND_CLASS(Fpzip) {
  construct();
  construct<nbind::Buffer>();
  method(nbytes);
  method(nvoxels);
  method(get_type);
  method(get_prec);
  method(get_nx);
  method(get_ny);
  method(get_nz);
  method(get_nf);
  method(decompress);
  method(dekempress);

  // debug
  // method(buflen);
}



