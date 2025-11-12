#include "Parts/Common.h"

#include "CREDITS_MAIN_Data.h"

#define tw_getpixel(b, x, y) b[(x) + (y) * SCREEN_WIDTH]
#define tw_putpixel(b, x, y, c) b[(x) + (y) * SCREEN_WIDTH] = (c)

namespace Credits
{
    constexpr const size_t FONAY = 32;
    namespace
    {
        const char * credits_fonaorder = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/?!:,.\"()+-";
        int credits_fonap[256]{};
        int credits_fonaw[256]{};

        char vram_split_top[SCREEN_WIDTH * 100]{};
        char vram_split_bottom[SCREEN_WIDTH * SCREEN_HEIGHT]{};

        char * split_vram = (char *)Shim::vram;
    }

    void reconstitute_split(int vertical, int horizontal)
    {
        memset(Shim::vram, 0, SCREEN_WIDTH * DOUBlE_SCREEN_HEIGHT);

        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            if (horizontal < 0)
            {
                memcpy(Shim::vram + SCREEN_WIDTH * i, vram_split_top + SCREEN_WIDTH * (i / 2) - horizontal, SCREEN_WIDTH + horizontal);
            }
            else
            {
                memcpy(Shim::vram + SCREEN_WIDTH * i + horizontal, vram_split_top + SCREEN_WIDTH * (i / 2), SCREEN_WIDTH - horizontal);
            }

            if (i + SCREEN_HEIGHT + vertical < DOUBlE_SCREEN_HEIGHT)
            {
                memcpy(Shim::vram + SCREEN_WIDTH * (i + SCREEN_HEIGHT + vertical), vram_split_bottom + SCREEN_WIDTH * i, SCREEN_WIDTH);
            }
        }
    }

    void credits_prt(int x, int y, const char * txt)
    {
        int x2w, x2, y2, y2w = y + FONAY, sx, d;

        while (*txt)
        {
            x2w = credits_fonaw[static_cast<size_t>(*txt)] + x;
            sx = credits_fonap[static_cast<size_t>(*txt)];

            for (x2 = x; x2 < x2w; x2++)
            {
                for (y2 = y; y2 < y2w; y2++)
                {
                    d = Data::credits_font[y2 - y][sx];
                    tw_putpixel(vram_split_bottom, x2, y2, static_cast<char>(d));
                }
                sx++;
            }

            x = x2 + 2;
            txt++;
        }
    }

    const char * credits_prtc(int x, int y, const char * txt)
    {
        int w = 0;
        const char * t = txt;

        while (*t)
            w += credits_fonaw[static_cast<size_t>(*t++)] + 2;

        credits_prt(x - w / 2, y, txt);

        return (t + 1);
    }

    void screenin(unsigned char * pic, const char * text)
    {
        int a, x, y, yy, v;

        memset(Shim::vram, 0, SCREEN_WIDTH * DOUBlE_SCREEN_HEIGHT);
        memset(vram_split_bottom, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        memset(vram_split_top, 0, SCREEN_WIDTH * 100);

        demo_vsync();

        Common::setpalarea((char *)&pic[16], 0, PaletteColorCount);

        pic = &pic[784];

        split_vram = (char *)(Shim::vram + SCREEN_WIDTH * SCREEN_HEIGHT);
        y = 16;

        while (*(text = (char *)credits_prtc(160, y, text)))
            y += FONAY + 10;

        split_vram = (char *)Shim::vram;

        for (x = 0; x < 160; x++)
            for (y = 0; y < 100; y++)
                tw_putpixel(vram_split_top, 80 + x, 0 + y, pic[y * 160 + x] + 16);

        for (y = 200 * 128; y > 0 && !demo_wantstoquit(); y = y * 12L / 13)
        {
            demo_vsync();

            yy = SCREEN_WIDTH - y / 80;
            for (a = 0; a < 10000; a++)
                ;

            reconstitute_split(y / 128, SCREEN_WIDTH - yy);
            demo_blit();
        }

        for (a = 0; a < 200 && !demo_wantstoquit(); a++)
        {
            demo_vsync();
            demo_blit();
        }

        for (y = 0, v = 0; y < 128 * 200 && !demo_wantstoquit(); y = y + v, v += 15)
        {
            demo_vsync();

            yy = SCREEN_WIDTH + y / 80;
            for (a = 0; a < 10000; a++)
                ;

            reconstitute_split(y / 128, SCREEN_WIDTH - yy);
            demo_blit();
        }
    }

    bool credits_init()
    {
        int x, y, a, b;

        for (x = 0; x < 1500 && *credits_fonaorder;)
        {
            while (x < 1500)
            {
                for (y = 0; y < static_cast<int>(FONAY); y++)
                    if (Data::credits_font[y][x]) break;
                if (y != FONAY) break;
                x++;
            }

            b = x;

            while (x < 1500)
            {
                for (y = 0; y < static_cast<int>(FONAY); y++)
                    if (Data::credits_font[y][x]) break;
                if (y == FONAY) break;
                x++;
            }

            credits_fonap[static_cast<size_t>(*credits_fonaorder)] = b;
            credits_fonaw[static_cast<size_t>(*credits_fonaorder)] = x - b;
            credits_fonaorder++;
        }

        credits_fonap[32] = 1500 - 32;
        credits_fonaw[32] = 8;

        memmove(&Data::credits_pic1[16 * 3 + 16], &Data::credits_pic1[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic2[16 * 3 + 16], &Data::credits_pic2[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic3[16 * 3 + 16], &Data::credits_pic3[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic4[16 * 3 + 16], &Data::credits_pic4[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic5[16 * 3 + 16], &Data::credits_pic5[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic5b[16 * 3 + 16], &Data::credits_pic5b[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic6[16 * 3 + 16], &Data::credits_pic6[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic7[16 * 3 + 16], &Data::credits_pic7[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic8[16 * 3 + 16], &Data::credits_pic8[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic9[16 * 3 + 16], &Data::credits_pic9[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic10[16 * 3 + 16], &Data::credits_pic10[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic10b[16 * 3 + 16], &Data::credits_pic10b[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic11[16 * 3 + 16], &Data::credits_pic11[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic12[16 * 3 + 16], &Data::credits_pic12[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic13[16 * 3 + 16], &Data::credits_pic13[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic14[16 * 3 + 16], &Data::credits_pic14[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic14b[16 * 3 + 16], &Data::credits_pic14b[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic15[16 * 3 + 16], &Data::credits_pic15[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic16[16 * 3 + 16], &Data::credits_pic16[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic17[16 * 3 + 16], &Data::credits_pic17[16], PaletteByteCount - 16 * 3);
        memmove(&Data::credits_pic18[16 * 3 + 16], &Data::credits_pic18[16], PaletteByteCount - 16 * 3);

        for (a = 0; a < 10; a++)
        {
            const char a7 = static_cast<char>(7 * a);

            Data::credits_pic1[a * 3 + 0 + 16] = Data::credits_pic1[a * 3 + 1 + 16] = Data::credits_pic1[a * 3 + 2 + 16] = a7;
            Data::credits_pic2[a * 3 + 0 + 16] = Data::credits_pic2[a * 3 + 1 + 16] = Data::credits_pic2[a * 3 + 2 + 16] = a7;
            Data::credits_pic3[a * 3 + 0 + 16] = Data::credits_pic3[a * 3 + 1 + 16] = Data::credits_pic3[a * 3 + 2 + 16] = a7;
            Data::credits_pic4[a * 3 + 0 + 16] = Data::credits_pic4[a * 3 + 1 + 16] = Data::credits_pic4[a * 3 + 2 + 16] = a7;
            Data::credits_pic5[a * 3 + 0 + 16] = Data::credits_pic5[a * 3 + 1 + 16] = Data::credits_pic5[a * 3 + 2 + 16] = a7;
            Data::credits_pic5b[a * 3 + 0 + 16] = Data::credits_pic5b[a * 3 + 1 + 16] = Data::credits_pic5b[a * 3 + 2 + 16] = a7;
            Data::credits_pic6[a * 3 + 0 + 16] = Data::credits_pic6[a * 3 + 1 + 16] = Data::credits_pic6[a * 3 + 2 + 16] = a7;
            Data::credits_pic7[a * 3 + 0 + 16] = Data::credits_pic7[a * 3 + 1 + 16] = Data::credits_pic7[a * 3 + 2 + 16] = a7;
            Data::credits_pic8[a * 3 + 0 + 16] = Data::credits_pic8[a * 3 + 1 + 16] = Data::credits_pic8[a * 3 + 2 + 16] = a7;
            Data::credits_pic9[a * 3 + 0 + 16] = Data::credits_pic9[a * 3 + 1 + 16] = Data::credits_pic9[a * 3 + 2 + 16] = a7;
            Data::credits_pic10[a * 3 + 0 + 16] = Data::credits_pic10[a * 3 + 1 + 16] = Data::credits_pic10[a * 3 + 2 + 16] = a7;
            Data::credits_pic10b[a * 3 + 0 + 16] = Data::credits_pic10b[a * 3 + 1 + 16] = Data::credits_pic10b[a * 3 + 2 + 16] = a7;
            Data::credits_pic11[a * 3 + 0 + 16] = Data::credits_pic11[a * 3 + 1 + 16] = Data::credits_pic11[a * 3 + 2 + 16] = a7;
            Data::credits_pic12[a * 3 + 0 + 16] = Data::credits_pic12[a * 3 + 1 + 16] = Data::credits_pic12[a * 3 + 2 + 16] = a7;
            Data::credits_pic13[a * 3 + 0 + 16] = Data::credits_pic13[a * 3 + 1 + 16] = Data::credits_pic13[a * 3 + 2 + 16] = a7;
            Data::credits_pic14[a * 3 + 0 + 16] = Data::credits_pic14[a * 3 + 1 + 16] = Data::credits_pic14[a * 3 + 2 + 16] = a7;
            Data::credits_pic14b[a * 3 + 0 + 16] = Data::credits_pic14b[a * 3 + 1 + 16] = Data::credits_pic14b[a * 3 + 2 + 16] = a7;
            Data::credits_pic15[a * 3 + 0 + 16] = Data::credits_pic15[a * 3 + 1 + 16] = Data::credits_pic15[a * 3 + 2 + 16] = a7;
            Data::credits_pic16[a * 3 + 0 + 16] = Data::credits_pic16[a * 3 + 1 + 16] = Data::credits_pic16[a * 3 + 2 + 16] = a7;
            Data::credits_pic17[a * 3 + 0 + 16] = Data::credits_pic17[a * 3 + 1 + 16] = Data::credits_pic17[a * 3 + 2 + 16] = a7;
            Data::credits_pic18[a * 3 + 0 + 16] = Data::credits_pic18[a * 3 + 1 + 16] = Data::credits_pic18[a * 3 + 2 + 16] = a7;
        }

        return true;
    }

    void main()
    {
        Shim::clearScreen();

        static bool once = credits_init();
        (void)once;

        if (!demo_wantstoquit())
            screenin(Data::credits_pic1, "GRAPHICS - MARVEL\0"
                                         "MUSIC - SKAVEN\0"
                                         "CODE - WILDFIRE\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic2,
                     "GRAPHICS - MARVEL\0"
                     "MUSIC - SKAVEN\0"
                     "CODE - PSI\0"
                     "OBJECTS - WILDFIRE\0" // --- W32 PORT CHANGE (updated from final) ---
            );

        if (!demo_wantstoquit())
            screenin(Data::credits_pic3, "GRAPHICS - MARVEL\0"
                                         "MUSIC - SKAVEN\0"
                                         "CODE - WILDFIRE\0"
                                         "ANIMATION - TRUG\0");

        if (!demo_wantstoquit()) screenin(Data::credits_pic4, "\0GRAPHICS - PIXEL\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic5, "GRAPHICS - PIXEL\0"
                                         "MUSIC - PURPLE MOTION\0"
                                         "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic5b, "\0MUSIC - PURPLE MOTION\0"
                                          "CODE - TRUG\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic6, "\0MUSIC - PURPLE MOTION\0"
                                         "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic7, "\0MUSIC - PURPLE MOTION\0"
                                         "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic8, "\0GRAPHICS - PIXEL\0"
                                         "MUSIC - PURPLE MOTION\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic9, "GRAPHICS - PIXEL\0"
                                         "MUSIC - PURPLE MOTION\0"
                                         "CODE - TRUG\0"
                                         "RENDERING - TRUG\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic10,
                     "SKETCH - SKAVEN\0" // --- W32 PORT CHANGE (updated from final) ---
                     "GRAPHICS - PIXEL\0"
                     "MUSIC - PURPLE MOTION\0"
                     "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic10b,
                     "SKETCH - SKAVEN\0" // --- W32 PORT CHANGE (updated from final) ---
                     "GRAPHICS - PIXEL\0"
                     "MUSIC - PURPLE MOTION\0"
                     "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic11, "\0MUSIC - PURPLE MOTION\0"
                                          "CODE - WILDFIRE\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic12, "\0MUSIC - PURPLE MOTION\0"
                                          "CODE - WILDFIRE\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic13, "\0MUSIC - PURPLE MOTION\0"
                                          "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic14, "GRAPHICS - PIXEL\0"
                                          "MUSIC - PURPLE MOTION\0"
                                          "CODE - TRUG\0"
                                          "RENDERING - TRUG\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic14b, "\0MUSIC - PURPLE MOTION\0"
                                           "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic15, "GRAPHICS - MARVEL\0"
                                          "MUSIC - PURPLE MOTION\0"
                                          "CODE - PSI\0");

        if (!demo_wantstoquit())
            screenin(Data::credits_pic16,
                     "MUSIC - SKAVEN\0"
                     "CODE - PSI\0"
                     "WORLD - TRUG\0" // --- W32 PORT CHANGE (updated from final) ---
            );

        if (!demo_wantstoquit())
            screenin(Data::credits_pic17,
                     "GRAPHICS - PIXEL\0"
                     "MUSIC - SKAVEN\0" // --- W32 PORT CHANGE (updated from final) ---
            );

        if (!demo_wantstoquit())
            screenin(Data::credits_pic18,
                     "GRAPHICS - PIXEL\0"
                     "MUSIC - SKAVEN\0" // --- W32 PORT CHANGE (updated from final) ---
                     "CODE - WILDFIRE\0");
    }
}