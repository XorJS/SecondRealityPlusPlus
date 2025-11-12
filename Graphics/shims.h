#pragma once

#include <stddef.h>

constexpr const float Default_JSSS = 0.80f;

#include "Blob/Blob.h"

void demo_blit();
void demo_changemode(int x, int y, float jsss = Default_JSSS);

namespace Shim
{
    constexpr const size_t VRAM_X = 640;
    constexpr const size_t VRAM_Y = 960;

    extern unsigned int palette[];
    extern unsigned int startpixel;
    extern unsigned char vram[];

    void clearScreen();

    void setpal(int idx, unsigned char r, unsigned char g, unsigned char b);
    void outp(int reg, unsigned int value);
    unsigned char inp(int reg);
    void setstartpixel(int reg);

    bool isDemoFirstPart();
    void finishedDemoFirstPart();
}