#include "Compression.h"

size_t Compression::compressRLE(const uint8_t* in, size_t inLen, uint8_t* out, size_t maxOutLen) {
    if (inLen == 0 || maxOutLen == 0) return 0;

    size_t inIdx = 0;
    size_t outIdx = 0;

    while (inIdx < inLen) {
        uint8_t count = 1;
        uint8_t current = in[inIdx];

        while (inIdx + count < inLen && in[inIdx + count] == current && count < 255) {
            count++;
        }

        if (outIdx + 2 > maxOutLen) return 0;

        out[outIdx++] = count;
        out[outIdx++] = current;

        inIdx += count;
    }

    return outIdx;
}

size_t Compression::decompressRLE(const uint8_t* in, size_t inLen, uint8_t* out, size_t maxOutLen) {
    if (inLen == 0 || inLen % 2 != 0 || maxOutLen == 0) return 0;

    size_t inIdx = 0;
    size_t outIdx = 0;

    while (inIdx < inLen) {
        uint8_t count = in[inIdx++];
        uint8_t current = in[inIdx++];

        if (outIdx + count > maxOutLen) return 0;

        for (uint8_t i = 0; i < count; i++) {
            out[outIdx++] = current;
        }
    }

    return outIdx;
}
