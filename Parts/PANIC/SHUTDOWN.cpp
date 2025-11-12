#include <math.h>

#include "Parts/Common.h"

#define tw_getpixel(x, y) shutdown_vram[(x) + (y / (shutdown_scandoubling ? 2 : 1)) * shutdown_scrwidth]
#define tw_putpixel(x, y, c) shutdown_vram[(x) + (y / (shutdown_scandoubling ? 2 : 1)) * shutdown_scrwidth] = (c)
#define tw_setpalette(p) Common::setpalarea(p)
#define tw_setrgbpalette(p, r, g, b) Shim::setpal(p, r, g, b)
#define tw_setstart(x) shutdown_offset = ((x) * 4)

namespace Shutdown
{
    namespace
    {
        char kuva[65000]{};

        char kuvapal[PaletteByteCount]{};
        char pal[PaletteByteCount]{};
        char fadepals[64][PaletteByteCount]{};

        unsigned int shutdown_offset = 0;
        unsigned int shutdown_scrwidth = 640;
        unsigned char shutdown_scandoubling = 0;
        unsigned char shutdown_vram[640 * 800]{};
    }

    void copyline(int from, int to, int count)
    {
        unsigned char * src = shutdown_vram + ((size_t)from << 2); // from * 4
        unsigned char * dst = shutdown_vram + ((size_t)to << 2);   // to   * 4
        size_t bytes = (size_t)count * 4;                          // 4 * count bytes

        memcpy(dst, src, bytes);
    }

    void resolve_shutdown_vram()
    {
        unsigned char * src = shutdown_vram + shutdown_offset;
        unsigned char * dst = Shim::vram;
        unsigned int height = shutdown_scandoubling ? SCREEN_HEIGHT : DOUBlE_SCREEN_HEIGHT;

        for (unsigned int i = 0; i < height; i++)
        {
            memcpy(dst, src, SCREEN_WIDTH);
            src += shutdown_scrwidth;
            dst += SCREEN_WIDTH;
            if (shutdown_scandoubling)
            {
                memcpy(dst, dst - SCREEN_WIDTH, SCREEN_WIDTH);
                dst += SCREEN_WIDTH;
            }
        }
    }

    void shutdown()
    {
        int x, y, a, b;

        for (a = 0; a < SCREEN_WIDTH; a++)
            tw_putpixel(a, 0, 0);

        for (a = 0; a < 64; a++)
            for (b = 3; b < static_cast<int>(PaletteByteCount); b++)
                fadepals[a][b] = static_cast<char>((a * 63 + kuvapal[b] * (64 - a)) / 64);

        for (y = 0; y < 100; y++)
            for (x = 0; x < SCREEN_WIDTH; x++)
                tw_putpixel(x, y + 150, tw_getpixel(x + SCREEN_WIDTH, y * 4));

        tw_setstart(100 * 160);

        demo_vsync();

        shutdown_scandoubling = 1;
        tw_setpalette(fadepals[3]);

        Shim::setpal(0, 63, 63, 63);

        tw_setstart(0);

        demo_vsync();

        demo_blit();

        tw_setpalette(fadepals[20]);
        shutdown_scrwidth = 1280;

        demo_vsync();
        demo_blit();

        for (a = 32; a > 2; a = a * 5 / 6)
        {
            demo_vsync();
            tw_setpalette(fadepals[63 - a]);

            for (b = a / 2; b <= a; b++)
            {
                copyline((0), (SCREEN_HEIGHT * 160 - (b * SCREEN_WIDTH)), PLANAR_WIDTH);
                copyline((0), (SCREEN_HEIGHT * 160 + (b * SCREEN_WIDTH)), PLANAR_WIDTH);
            }

            for (b = 0; b < a; b++)
                copyline((PLANAR_WIDTH + (DOUBlE_SCREEN_HEIGHT * b / a) * 160), (SCREEN_HEIGHT * 160 + (b - a / 2) * SCREEN_WIDTH), PLANAR_WIDTH);

            resolve_shutdown_vram();
            demo_blit();
        }

        copyline((0), (202 * 160), PLANAR_WIDTH);
        copyline((0), (198 * 160), PLANAR_WIDTH);

        for (x = 20; x <= 160; x += 3)
        {
            demo_vsync();

            tw_putpixel(x, SCREEN_HEIGHT, 0);
            tw_putpixel(SCREEN_WIDTH - x, SCREEN_HEIGHT, 0);
            tw_putpixel(x + 1, SCREEN_HEIGHT, 0);
            tw_putpixel((SCREEN_WIDTH - 1) - x, SCREEN_HEIGHT, 0);
            tw_putpixel(x + 2, SCREEN_HEIGHT, 0);
            tw_putpixel(318 - x, SCREEN_HEIGHT, 0);
            tw_putpixel(x + 3, SCREEN_HEIGHT, 0);
            tw_putpixel(317 - x, SCREEN_HEIGHT, 0);
            resolve_shutdown_vram();
            demo_blit();
        }

        tw_putpixel(160, 200, 1);

        for (a = 0; a < 60; a++)
        {
            demo_vsync();

            b = static_cast<int>(cos(a / 120.0 * 3 * 2 * 3.1415926535) * 31.0 + 32);

            tw_setrgbpalette(1, static_cast<char>(b), static_cast<char>(b), static_cast<char>(b));
            resolve_shutdown_vram();
            demo_blit();
        }
    }

    void main()
    {
        CLEAR(kuva);
        CLEAR(kuvapal);
        CLEAR(pal);
        CLEAR(fadepals);

        shutdown_offset = 0;
        shutdown_scrwidth = 640;
        shutdown_scandoubling = 0;
        CLEAR(shutdown_vram);

        for (int y = 0; y < DOUBlE_SCREEN_HEIGHT; y++)
        {
            memcpy(shutdown_vram + y * 640 + SCREEN_WIDTH, Shim::vram + y * SCREEN_WIDTH, SCREEN_WIDTH);
        }

        Common::getpalarea(kuvapal);

        shutdown_scrwidth = 640;

        shutdown();
    }

}
