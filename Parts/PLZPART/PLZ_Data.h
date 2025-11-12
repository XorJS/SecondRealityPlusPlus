#pragma once

#include <stdint.h>
#include <string.h>

namespace PLZ::Data
{
    constexpr const size_t sinBufferSize = 16384;

    extern const char ptau[256];
    extern const unsigned char psini[sinBufferSize];
    extern const short lsini4[sinBufferSize / 2];
    extern const short lsini16[sinBufferSize / 2];
    extern const unsigned char _sinit[];
    extern const unsigned char _kosinit[];
    extern const int inittable[10][8];
    extern const short splinecoef[1024];
    extern const short buu[][8];
    extern const uint16_t plz_dtau[65];
}
