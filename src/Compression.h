#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <Arduino.h>

class Compression {
public:
    static size_t compressRLE(const uint8_t* in, size_t inLen, uint8_t* out, size_t maxOutLen);
    static size_t decompressRLE(const uint8_t* in, size_t inLen, uint8_t* out, size_t maxOutLen);
};

#endif // COMPRESSION_H
