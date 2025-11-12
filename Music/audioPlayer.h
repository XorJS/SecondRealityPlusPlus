#pragma once

#include "st3play.h"

namespace AudioPlayer
{
    uint32_t getSamplePosition();
    bool openMixer(uint32_t audioFreq, int32_t & samplesLeft, uint32_t & totalSampleCount);
    void closeMixer();

    void Update(bool yield = false);
}
