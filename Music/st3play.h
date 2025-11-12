#pragma once

#include <stdint.h>

#define MIX_BUF_SAMPLES 4096

namespace st3play
{
    bool PlaySong(const unsigned char * moduleData, unsigned int dataLength, bool useInterpolationFlag, unsigned int audioFreq, unsigned int startingOrder);
    void Close();
    void GetOrderRowAndFrame(unsigned short * orderPtr, unsigned short * rowPtr, unsigned int * framePtr);
    unsigned short GetOrder();
    unsigned short GetRow();
    unsigned int GetFrame();
    short GetPlusFlags();

    bool FillAudioBuffer(int16_t * audioBuffer, int32_t samples);
}
