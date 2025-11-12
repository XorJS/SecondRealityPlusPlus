#pragma once

#include <cstring>
#include <stdint.h>

#include "Graphics/shims.h"
#include "Music/Music.h"

#include "Music/audioPlayer.h"

bool demo_wantstoquit();
int demo_vsync(bool updateAudio = false);

#define COUNT(X) (sizeof(X) / sizeof(X[0]))
#define CLEAR(X) memset(X, 0, sizeof(X));

constexpr const int PaletteColorCount = 256;
constexpr const int PaletteByteCount = (3 * PaletteColorCount);

constexpr const int VIRTUAL_SCREEN_WIDTH = 640;
constexpr const int VIRTUAL_SCREEN_HEIGHT = 400;

constexpr const int32_t SCREEN_WIDTH = 320;
constexpr const int32_t SCREEN_HEIGHT = 200;
constexpr const int32_t SCREEN_SIZE = (SCREEN_WIDTH * SCREEN_HEIGHT);

constexpr const int32_t DOUBlE_SCREEN_WIDTH = (2 * SCREEN_WIDTH);
constexpr const int32_t DOUBlE_SCREEN_HEIGHT = (2 * SCREEN_HEIGHT);
constexpr const int32_t DOUBlE_SCREEN_SIZE = (DOUBlE_SCREEN_WIDTH * DOUBlE_SCREEN_HEIGHT);

constexpr const int32_t PLANAR_WIDTH = 80;

using Palette = char[PaletteByteCount];

namespace Common
{
    namespace Data
    {
        extern const short sin1024[];
    }

    extern int frame_count;
    extern char * cop_pal;
    extern int do_pal;
    extern int cop_start;
    extern int cop_scrl;
    extern int cop_dofade;
    extern int cop_drop;
    extern short * cop_fadepal;
    extern Palette fadepal;
    extern short fadepal_short[];

    void reset();

    void copper2();
    void copper3();

    void readp(char * dest, int row, const char * src);

    void setpalarea(char * p, int offset = 0, int count = PaletteColorCount);
    void getpalarea(char * p, int offset = 0, int count = PaletteColorCount);
}
