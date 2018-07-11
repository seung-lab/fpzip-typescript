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
protected:
  nbind::Buffer* compressedptr;
public:
  unsigned int type;
  unsigned int prec;
  size_t nx;
  size_t ny;
  size_t nz;
  size_t nf;

  Fpzip (nbind::Buffer buf) {
    type = 0;
    prec = 0;
    nx = 0;
    ny = 0;
    nz = 0;
    nf = 0;
    compressedptr = &buf;
    decode_headers();
  }

  Fpzip(Fpzip &orig) {
    type = orig.type;
    prec = orig.prec;
    nx = orig.nx;
    ny = orig.ny;
    nz = orig.nz;
    nf = orig.nf;
    compressedptr = orig.compressedptr;
  }

  ~Fpzip() {
    free(compressedptr);
  }

  size_t nvoxels() {
    return nx * ny * nz * nf;
  }

  size_t nbytes() {
    // type: 0 = float, 1 = double
    return nvoxels() * (type + 1) * sizeof(float);
  }

  size_t get_type() {
    return type;
  }

  void decode_headers() {
    unsigned char* data = compressedptr->data();

    char *errorstr = new char[128];

    FPZ* fpz = fpzip_read_from_buffer(data);
    if (!fpzip_read_header(fpz)) {
      sprintf(errorstr, "cannot read header: %s\n", fpzip_errstr[fpzip_errno]);
      printf(errorstr, '\n');
      throw errorstr;
    }
    type = fpz->type;
    prec = fpz->prec;
    nx = fpz->nx;
    ny = fpz->ny;
    nz = fpz->nz;
    nf = fpz->nf;
    
    fpzip_read_close(fpz);
    free(fpz);
    free(errorstr);
  }

  void check_length(nbind::Buffer &buf) {
    char *errorstr = new char[512];

    if (buf.length() < nbytes()) {
      sprintf(errorstr, "Javascript TypedArray (%u bytes) should be at least %u bytes long.", 
        (unsigned int)buf.length(), (unsigned int)nbytes());
      printf(errorstr, '\n');
      throw errorstr;
    }

    free(errorstr);
  }

  void decompress(nbind::Buffer &buf) {
    check_length(buf);

    unsigned char *data = buf.data();

    if (type == FPZIP_TYPE_FLOAT) {
      dfpz<float>(data);
    }
    else {
      dfpz<double>(data);
    }

    buf.commit();
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
  void dekempress(nbind::Buffer &buf) {
    check_length(buf);

    unsigned char *data = buf.data();

    if (type == FPZIP_TYPE_FLOAT) {
      dfpz<float>(data);
      dekempress_algo<float>( (float*)data );
    }
    else {
      dfpz<double>(data);
      dekempress_algo<double>( (double*)data );
    }

    buf.commit();
  }

  /* Standard fpzip decompression. 
  * 
  * Example:
  * DecodedImage *di = decompress(buffer);
  * float* img = (float*)di->data;
  */
  template <typename T>
  void dfpz(unsigned char *jsdata) {
    char *errorstr = new char[128];
    
    T *data = (T*)jsdata;
 
    unsigned char* buffer = compressedptr->data();

    FPZ* fpz = fpzip_read_from_buffer((T*)buffer);

    // perform actual decompression
    if (!fpzip_read(fpz, data)) {
      sprintf(errorstr, "decompression failed: %s\n", fpzip_errstr[fpzip_errno]);
      printf(errorstr, '\n');
      throw errorstr;
    }

    fpzip_read_close(fpz);

    free(fpz);
    free(errorstr);
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
  construct<nbind::Buffer>();
  method(nbytes);
  method(nvoxels);
  method(get_type);
  method(decompress);
  method(dekempress);
}



