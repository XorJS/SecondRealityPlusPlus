#include "Parts/Common.h"

#include "Forest_Data.h"

namespace Forest
{
    static void Putrouts_from_stream(const uint8_t * pos_bytes, const uint8_t * font)
    {
        const uint8_t * bg = &Data::forest_hback[778];
        const uint8_t * p = pos_bytes;

        for (uint32_t blocks = 237u * 31u; blocks--; font++)
        {
            uint16_t count = (uint16_t)(p[0] | (p[1] << 8));
            p += 2;

            if (count != 0)
            {
                for (uint16_t i = 0; i < count; ++i)
                {
                    uint16_t dest = (uint16_t)(p[0] | (p[1] << 8));
                    p += 2;

                    Shim::vram[dest] = (uint8_t)(bg[dest] + *font);
                }
            }
        }
    }

    void main()
    {
        uint8_t fbuf[640 * 31] = { 0 };
        uint8_t font[237 * 31] = { 0 };

        uint8_t pal[PaletteByteCount] = { 0 };
        uint8_t fpal[PaletteByteCount] = { 0 };
        uint8_t tmppal[PaletteByteCount] = { 0 };

        uint16_t scp = 0;
        uint16_t sss = 0;
        uint8_t fp = 0;
        const uint16_t veke = 2800;
        uint16_t frame = 0;
        int quit = 0;
        int fadeout = 0;

        for (int x = 0; x < 31; ++x)
        {
            memcpy(&fbuf[x * 640], &Data::forest_o2[x * 640 + 778], 640);
        }

        for (size_t i = 0; i < sizeof(fbuf); ++i)
        {
            if (fbuf[i] > 0) fbuf[i] = (uint8_t)(fbuf[i] + 128);
        }

        memcpy(pal, &Data::forest_hback[10], PaletteByteCount);
        memcpy(Shim::vram, &Data::forest_hback[778], SCREEN_SIZE);

        for (int i = 0; i < PaletteColorCount; ++i)
            Shim::setpal(i, 0, 0, 0);

        memcpy(tmppal, pal, PaletteByteCount);
        memset(tmppal + 0, 0, 32 * 3);
        memset(tmppal + 128 * 3, 0, 32 * 3);
        memset(fpal, 0, PaletteByteCount);

        for (int y = 0; y < 64; ++y)
        {
            demo_vsync();
            for (int x = 0; x < PaletteColorCount; ++x)
            {
                Shim::setpal(x, fpal[x * 3 + 0], fpal[x * 3 + 1], fpal[x * 3 + 2]);
            }

            for (int i = 0; i < PaletteByteCount; ++i)
            {
                if (fpal[i] < tmppal[i]) ++fpal[i];
            }

            demo_blit();
        }

        memcpy(tmppal, pal, PaletteByteCount);
        memcpy(fpal, pal, PaletteByteCount);
        memset(fpal + 0, 0, 32 * 3);
        memset(fpal + 128 * 3, 0, 32 * 3);

        for (int x = 0; x < PaletteColorCount; ++x)
        {
            Shim::setpal(x, fpal[x * 3 + 0], fpal[x * 3 + 1], fpal[x * 3 + 2]);
        }

        for (int row = 0; row < 31; ++row)
        {
            memcpy(&font[row * 237 + 104], &fbuf[row * 640 + 0], 133);
        }

        scp = 133;

        while (Music::getPlusFlags() > 0)
        {
            demo_blit();
        }

        sss = 0;

        for (int y = 0; y < 63 * 2; ++y)
        {
            demo_vsync();

            if (y & 1)
            {
                for (int x = 0; x <= 176; ++x)
                    Shim::setpal(x, fpal[x * 3 + 0], fpal[x * 3 + 1], fpal[x * 3 + 2]);

                for (int i = 0; i <= 176; ++i)
                {
                    int idx = i * 3;
                    if (fpal[idx + 0] < tmppal[idx + 0]) ++fpal[idx + 0];
                    if (fpal[idx + 1] < tmppal[idx + 1]) ++fpal[idx + 1];
                    if (fpal[idx + 2] < tmppal[idx + 2]) ++fpal[idx + 2];
                }
            }

            switch (sss)
            {
                case 0:
                    Putrouts_from_stream(Data::forest_posi1, font);
                    break;
                case 1:
                    Putrouts_from_stream(Data::forest_posi2, font);
                    break;
                default: {
                    Putrouts_from_stream(Data::forest_posi3, font);
                    memmove(&font[0], &font[1], 237u * 31u - 1u);

                    for (int row = 0; row < 31; ++row)
                    {
                        font[row * 237 + 236] = fbuf[row * 640 + scp];
                    }

                    if (scp < 639) ++scp;

                    break;
                }
            }

            sss = (uint16_t)((sss == 2) ? 0 : (sss + 1));

            demo_blit();
        }

        memset(tmppal, 0, PaletteByteCount);

        frame = 0;
        quit = 0;
        fp = 0;
        fadeout = 0;

        do
        {
            demo_vsync();

            if (Music::getPlusFlags() == -11) fadeout = 1;

            if (fadeout)
            {
                if (fp == 64)
                    quit = 1;
                else
                    ++fp;

                for (int x = 0; x < PaletteColorCount; ++x)
                {
                    Shim::setpal(x, fpal[x * 3 + 0], fpal[x * 3 + 1], fpal[x * 3 + 2]);
                    int i0 = x * 3 + 0, i1 = x * 3 + 1, i2 = x * 3 + 2;
                    if (fpal[i0] > tmppal[i0]) --fpal[i0];
                    if (fpal[i1] > tmppal[i1]) --fpal[i1];
                    if (fpal[i2] > tmppal[i2]) --fpal[i2];
                }
            }

            switch (sss)
            {
                case 0:
                    Putrouts_from_stream(Data::forest_posi1, font);
                    break;
                case 1:
                    Putrouts_from_stream(Data::forest_posi2, font);
                    break;
                default: {
                    Putrouts_from_stream(Data::forest_posi3, font);
                    memmove(&font[0], &font[1], 237u * 31u - 1u);

                    for (int row = 0; row < 31; ++row)
                    {
                        font[row * 237 + 236] = fbuf[row * 640 + scp];
                    }

                    if (scp < 639) ++scp;

                    break;
                }
            }
            sss = (uint16_t)((sss == 2) ? 0 : (sss + 1));

            ++frame;

            demo_blit();

        } while (!demo_wantstoquit() && frame != veke && !quit);
    }
}
