#include <math.h>

#include "Parts/Common.h"

#include "Tunneli_Data.h"

namespace Tunneli
{
    constexpr const size_t FRAME_COUNT_TO_EXIT_VEKE = 1060;

    namespace
    {
        struct bc
        {
            int16_t x, y;
        };
        struct rengas
        {
            int32_t x, y;
            uint8_t c;
        };

        const unsigned short * tunnel_tun = (const unsigned short *)Data::_tunnel_tun;
        const unsigned short * tunnel_sini = (const unsigned short *)Data::_tunnel_sini;

        rengas putki[103]{};
        bc pcalc[138][64]{};
        uint16_t rows[501]{};
        uint16_t sinit[4097]{};
        uint16_t cosit[2049]{};
        uint16_t sade[103]{};
        uint16_t oldpos[7501]{};

        int32_t x = 0, y = 0, z = 0;
        uint16_t sx = 0, sy = 0;
        uint16_t frame = 0;

        bool quit = false;
    }

    void InitTunneli()
    {
        x = 0, y = 0, z = 0;
        sx = 0, sy = 0;
        frame = 0;
        quit = false;

        CLEAR(putki);
        CLEAR(pcalc);
        CLEAR(sinit);
        CLEAR(cosit);
        CLEAR(sade);

        for (int i = 0; i < static_cast<int>(COUNT(rows)); ++i)
        {
            rows[i] = (i < 100 || i >= 300) ? SCREEN_SIZE : (uint16_t)((i - 100) * SCREEN_WIDTH);
        }

        memcpy(&sinit[0], &tunnel_sini[0], 4097 * sizeof(uint16_t));
        const uint16_t * cosptr = &tunnel_sini[4097];
        memcpy(&cosit[0], cosptr, 2048 * sizeof(uint16_t));

        memcpy(&pcalc[0][0], &tunnel_tun[0], sizeof(pcalc));

        for (int i = 0; i < static_cast<int>(PaletteColorCount); ++i)
        {
            Shim::setpal(i, 0, 0, 0);
        }

        for (int i = 0; i <= 64; ++i)
        {
            Shim::setpal(64 + i, (unsigned char)(64 - i), (unsigned char)(64 - i), (unsigned char)(64 - i));
        }

        for (int i = 0; i <= 64; ++i)
        {
            Shim::setpal(128 + i, (unsigned char)((64 - i) * 3 / 4), (unsigned char)((64 - i) * 3 / 4), (unsigned char)((64 - i) * 3 / 4));
        }

        Shim::setpal(68, 0, 0, 0);
        Shim::setpal(132, 0, 0, 0);
        Shim::setpal(255, 0, 63, 0);

        for (int i = 0; i <= 100; ++i)
        {
            putki[i].x = 0;
            putki[i].y = 0;
            putki[i].c = 0;
        }

        for (z = 0; z <= 100; ++z)
        {
            sade[z] = (uint16_t)(16384 / ((z * 7) + 95));
        }

        for (int i = 0; i < static_cast<int>(COUNT(oldpos)); ++i)
        {
            oldpos[i] = SCREEN_SIZE;
        }
    }

    void tunneli()
    {
        do
        {
            uint16_t frames = static_cast<uint16_t>(demo_vsync());

            uint16_t ry = 0;
            for (int xi = 80; xi >= 4; --xi)
            {
                int16_t bx = (int16_t)(putki[xi].x - putki[5].x);
                int16_t by = (int16_t)(putki[xi].y - putki[5].y);
                uint16_t br = sade[xi];
                uint8_t bbc = (uint8_t)(putki[xi].c + (uint8_t)lround(static_cast<double>(xi) / 1.3));
                bc * pcp = &pcalc[br][0];

                if (bbc >= 64)
                {
                    uint16_t ax = ry;
                    for (int i = 0; i < 64; ++i, ax++)
                    {
                        const uint16_t idx = ax < (COUNT(oldpos) - 1) ? ax : (COUNT(oldpos) - 1);

                        if (uint16_t prev = oldpos[idx]; prev < SCREEN_SIZE)
                        {
                            Shim::vram[prev] = 0;
                        }

                        int newpos = SCREEN_SIZE;

                        int di = (int)pcp[i].x + bx;
                        if ((unsigned)di <= (SCREEN_WIDTH - 1))
                        {
                            int yv = (int)pcp[i].y + by;
                            int row = 100 + yv;
                            if ((unsigned)row <= 500u)
                            {
                                uint16_t base = rows[row];
                                if (base < (SCREEN_SIZE))
                                {
                                    newpos = base + di;
                                    Shim::vram[newpos] = bbc;
                                }
                            }
                        }

                        oldpos[idx] = (uint16_t)newpos;
                    }
                    ry = ax;
                }
            }

            for (uint16_t sync = 0; sync < frames; ++sync)
            {
                putki[100].x = (int16_t)(cosit[sy & 2047]) - (int16_t)(sinit[(sy * 3) & 4095]) - (int16_t)(cosit[sx & 2047]);
                putki[100].y = (int16_t)(sinit[(sx * 2) & 4095]) - (int16_t)(cosit[sx & 2047]) + (int16_t)(sinit[y & 4095]);

                memcpy(&putki[0], &putki[1], 100 * sizeof(rengas));

                ++sy;
                ++sx;

                putki[99].c = ((sy & 15) > 7) ? 128 : 64;

                if (frame >= (uint16_t)(FRAME_COUNT_TO_EXIT_VEKE - 102))
                {
                    putki[99].c = 0;
                }

                if (++frame == FRAME_COUNT_TO_EXIT_VEKE || demo_wantstoquit())
                {
                    quit = true;
                    break;
                }
            }

            demo_blit();
        } while (!quit);
    }

    void main()
    {
        InitTunneli();
        tunneli();
    }
}
