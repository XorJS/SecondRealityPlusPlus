#pragma once

#include <stdint.h>
#include <string.h>

namespace Coman::Data
{
    extern const unsigned char w1dta[];
    extern const unsigned char w2dta[];

    struct TheLoopBlock
    {
        uint8_t mode; // 1 = THELOOP_BLOCK, 2 = THELOOP2_BLOCK
        int16_t bx_add;
        int16_t dx_add;
        uint32_t eax_inc;
    };

    extern const TheLoopBlock kBlocks[];
    extern const size_t kBlocks_Size;
}