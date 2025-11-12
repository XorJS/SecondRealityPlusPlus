#include "Parts/Common.h"

#include "Water_Data.h"

namespace Water
{
    const size_t BACKGROUND_OFFSET = 778;
    const size_t FB_W = 158;
    const size_t FB_H = 34;
    const size_t FB_SIZE = FB_W * FB_H;

    namespace
    {
        Palette pal{};
        Palette tmppal{};

        uint8_t font[400 * 35]{};
        uint8_t fbuf[FB_SIZE]{};

        uint16_t frames = 0;
    }

    static void Putrouts1_C(const uint8_t * srcBytes)
    {
        const uint16_t * src = (const uint16_t *)srcBytes;
        const uint8_t * bg = Data::water_tausta + BACKGROUND_OFFSET;

        for (size_t index = 0; index < FB_SIZE; ++index)
        {
            uint16_t n = *src++;
            if (n != 0)
            {
                uint8_t v = fbuf[index];
                for (uint16_t k = 0; k < n; ++k)
                {
                    uint16_t off = *src++;
                    uint8_t outv = (v != 0) ? v : bg[off];
                    Shim::vram[off] = outv;
                }
            }
        }
    }

    static void scr(uint8_t pos)
    {
        switch (pos)
        {
            case 0:
                Putrouts1_C(Data::water_wat1);
                break;
            case 1:
                Putrouts1_C(Data::water_wat2);
                break;
            case 2:
                Putrouts1_C(Data::water_wat3);
                break;
            default:
                break;
        }
    }

    void main()
    {
        CLEAR(pal);
        CLEAR(tmppal);

        CLEAR(font);
        CLEAR(fbuf);

        frames = 0;

        uint16_t x, y;
        uint16_t co = 0;
        bool fadeout = false;
        uint16_t quit = 0;
        uint16_t fp = 0;
        uint16_t pf = 0;
        uint16_t scp = 0;
        uint16_t sss = 0;

        if (!Shim::isDemoFirstPart())
        {
            while (Music::getPlusFlags() <= 0)
            {
                AudioPlayer::Update(true);
            }
        }

        co = (uint16_t)Music::getOrder();

        memset(fbuf, 0, sizeof(fbuf));

        memcpy(pal, Data::water_miekka + 10, PaletteByteCount);
        memcpy(font, Data::water_miekka + BACKGROUND_OFFSET, 400 * 34);

        for (x = 0; x < PaletteColorCount; ++x)
            Shim::setpal((uint8_t)x, 0, 0, 0);

        memcpy((void *)Shim::vram, (const void *)(Data::water_tausta + BACKGROUND_OFFSET), SCREEN_SIZE);

        memcpy(tmppal, pal, PaletteByteCount);
        memset(pal, 0, PaletteByteCount);

        for (y = 0; y < 63 * 2; ++y)
        {
            frames = (uint16_t)demo_vsync();

            if ((y & 1) == 1)
            {
                for (x = 0; x < PaletteColorCount; ++x)
                {
                    Shim::setpal((uint8_t)x, pal[x * 3 + 0], pal[x * 3 + 1], pal[x * 3 + 2]);

                    for (pf = 0; pf < 3; ++pf)
                    {
                        uint16_t idx = (uint16_t)(x * 3 + pf);
                        if (pal[idx] < tmppal[idx]) pal[idx]++;
                    }
                }
            }

            scr((uint8_t)sss);

            sss = (sss == 2) ? 0 : (uint16_t)(sss + 1);

            demo_blit();
        }

        while (true)
        {
            demo_blit();

            if (demo_wantstoquit()) break;

            int order = Music::getOrder();
            int row = Music::getRow();

            if (!(order == (int)co || row < 16)) break;
        }

        sss = 0;
        scp = 0;
        quit = 0;
        fp = 0;
        fadeout = false;
        memset(tmppal, 0, PaletteByteCount);

        while (!demo_wantstoquit() && !quit)
        {
            frames = (uint16_t)demo_vsync();

            if (Music::getPlusFlags() == -11) fadeout = true;

            if (fadeout)
            {
                if (fp == 64)
                {
                    quit = 1;
                }
                else
                {
                    fp++;
                }

                for (x = 0; x < PaletteColorCount; ++x)
                {
                    Shim::setpal((uint8_t)x, pal[x * 3 + 0], pal[x * 3 + 1], pal[x * 3 + 2]);

                    for (pf = 0; pf < 3; ++pf)
                    {
                        uint16_t idx = (uint16_t)(x * 3 + pf);
                        if (pal[idx] > tmppal[idx]) pal[idx]--;
                    }
                }
            }

            scr((uint8_t)sss);

            if (sss == 2)
            {
                sss = 0;

                memmove(&fbuf[0], &fbuf[1], sizeof(fbuf) - 1);

                for (x = 0; x < FB_H; ++x)
                {
                    fbuf[FB_W - 0 + x * FB_W] = font[x * 400 + scp];
                }

                if (scp < 390) scp++;
            }
            else
            {
                sss++;
            }

            demo_blit();
        }
    }
}