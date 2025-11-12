#include "Parts/Common.h"

#include "ALKU_MAIN_Data.h"

namespace Alku
{
    constexpr const int FONT_STRIDE = 1500;
    constexpr const int SCRLF = 9;

    namespace
    {
        const char * alku_fonaorder = "ABCDEFGHIJKLMNOPQRSTUVWXabcdefghijklmnopqrstuvwxyz0123456789"
                                      "!?,.:\x8f\x8f()+-*='\x8f\x99";

        char font[FONT_STRIDE * 31 + 1500 * 5 * 2]{};

        unsigned char alku_planar_vram[352 * 500]{};

        Palette palette{};  // pic
        Palette palette2{}; // pic & text
        Palette fuckpal{};
        Palette fade1{}; // black
        Palette fade2{}; // text

        short picin[PaletteByteCount]{};
        short textin[PaletteByteCount]{};
        short textout[PaletteByteCount]{};

        char cfpal[PaletteByteCount * 2]{};

        int alku_fonap[256]{};
        int alku_fonaw[256]{};

        short alku_dtau[30000]{};
        char tbuf[186][352]{};

        int a = 0, p = 0, alku_tptr = 0;
    }

    void ascrolltext(unsigned short scrl, const void * text)
    {
        const size_t BASE = 100u * 352u;

        for (const unsigned short * pText = (const unsigned short *)text;;)
        {
            for (int i = 0; i < 20; ++i)
            {
                unsigned short idx = pText[0];
                if (idx == 0xFFFFu)
                {
                    return;
                }

                const size_t offset = BASE + (size_t)scrl + (size_t)idx;

                unsigned short val = pText[1];

                alku_planar_vram[offset - 1] ^= (unsigned char)(val & 0xFF);

                pText += 2;
            }
        }
    }

    void outline(const void * src, void * dest)
    {
        const size_t SRC_STRIDE = 640u;
        const size_t DST_STRIDE = 352u * 2u;
        const size_t LINES = 75u;
        const size_t BLOCK_SHIFT = LINES * SRC_STRIDE;
        const size_t DST_BLOCK = LINES * DST_STRIDE;

        const unsigned char * srcBase = (const unsigned char *)src;
        unsigned char * destBase = (unsigned char *)dest;

        for (int offset = 4; offset >= 1; --offset)
        {
            const unsigned char * currentSrc = srcBase + offset;
            unsigned char * currentDest = destBase + offset;

            for (size_t ccc = 0; ccc < LINES; ++ccc)
            {
                currentDest[ccc * DST_STRIDE] = currentSrc[ccc * SRC_STRIDE];
            }

            currentSrc += BLOCK_SHIFT;

            for (size_t ccc = 0; ccc < LINES; ++ccc)
            {
                currentDest[DST_BLOCK + ccc * DST_STRIDE] = currentSrc[ccc * SRC_STRIDE];
            }
        }
    }

    void alku_simulate_scroll()
    {
        for (int y = 0; y < DOUBlE_SCREEN_HEIGHT; y++)
        {
            memcpy(Shim::vram + y * SCREEN_WIDTH, alku_planar_vram + Common::cop_start * 4 + Common::cop_scrl + y * 352, SCREEN_WIDTH);
        }
    }

    void alku_init()
    {
        int b, x, y;

        Shim::clearScreen();

        Common::setpalarea(fade1, 0, PaletteColorCount);

        memcpy(palette, Data::hzpic + 16, PaletteByteCount);

        for (a = 0; a < 88; a++)
        {
            outline(Data::hzpic + a * 4 + 784, alku_planar_vram + 4 * (a + 176 * 25));
            outline(Data::hzpic + a * 4 + 784, alku_planar_vram + 4 * (a + 176 * 25 + 88));
        }

        alku_simulate_scroll();

        memset(font, 0, COUNT(font));

        memcpy(font, Data::alkuFont, 45001);

        for (y = 0; y < 32; y++)
        {
            for (a = 0; a < FONT_STRIDE; a++)
            {
                switch (font[y * FONT_STRIDE + a] & 3)
                {
                    case 0x1:
                        b = 0x40;
                        break;
                    case 0x2:
                        b = 0x80;
                        break;
                    case 0x3:
                        b = 0xc0;
                        break;
                    default:
                        b = 0;
                }

                font[y * FONT_STRIDE + a] = static_cast<char>(b);
            }
        }

        for (y = 0; y < PaletteByteCount; y += 3)
        {
            if (y < 64 * 3)
            {
                palette2[y + 0] = palette[y + 0];
                palette2[y + 1] = palette[y + 1];
                palette2[y + 2] = palette[y + 2];
            }
            else if (y < 128 * 3)
            {
                palette2[y + 0] = ((fade2[y + 0] = palette[0x1 * 3 + 0]) * 63 + palette[y % (64 * 3) + 0] * (63 - palette[0x1 * 3 + 0])) >> 6;
                palette2[y + 1] = ((fade2[y + 1] = palette[0x1 * 3 + 1]) * 63 + palette[y % (64 * 3) + 1] * (63 - palette[0x1 * 3 + 1])) >> 6;
                palette2[y + 2] = ((fade2[y + 2] = palette[0x1 * 3 + 2]) * 63 + palette[y % (64 * 3) + 2] * (63 - palette[0x1 * 3 + 2])) >> 6;
            }
            else if (y < 192 * 3)
            {
                palette2[y + 0] = ((fade2[y + 0] = palette[0x2 * 3 + 0]) * 63 + palette[y % (64 * 3) + 0] * (63 - palette[0x2 * 3 + 0])) >> 6;
                palette2[y + 1] = ((fade2[y + 1] = palette[0x2 * 3 + 1]) * 63 + palette[y % (64 * 3) + 1] * (63 - palette[0x2 * 3 + 1])) >> 6;
                palette2[y + 2] = ((fade2[y + 2] = palette[0x2 * 3 + 2]) * 63 + palette[y % (64 * 3) + 2] * (63 - palette[0x2 * 3 + 2])) >> 6;
            }
            else
            {
                palette2[y + 0] = ((fade2[y + 0] = palette[0x3 * 3 + 0]) * 63 + palette[y % (64 * 3) + 0] * (63 - palette[0x3 * 3 + 0])) >> 6;
                palette2[y + 1] = ((fade2[y + 1] = palette[0x3 * 3 + 1]) * 63 + palette[y % (64 * 3) + 1] * (63 - palette[0x3 * 3 + 1])) >> 6;
                palette2[y + 2] = ((fade2[y + 2] = palette[0x3 * 3 + 2]) * 63 + palette[y % (64 * 3) + 2] * (63 - palette[0x3 * 3 + 2])) >> 6;
            }
        }

        for (a = 192; a < PaletteByteCount; a++)
        {
            palette[a] = palette[a - 192];
        }

        for (x = 0; x < FONT_STRIDE && *alku_fonaorder;)
        {
            while (x < FONT_STRIDE)
            {
                for (y = 0; y < 32; y++)
                    if (font[y * FONT_STRIDE + x]) break;

                if (y != 32) break;

                x++;
            }

            b = x;

            while (x < FONT_STRIDE)
            {
                for (y = 0; y < 32; y++)
                    if (font[y * FONT_STRIDE + x]) break;

                if (y == 32) break;

                x++;
            }

            alku_fonap[(unsigned char)*alku_fonaorder] = b;
            alku_fonaw[(unsigned char)*alku_fonaorder] = x - b;

            alku_fonaorder++;
        }

        alku_fonap[32] = FONT_STRIDE - 20;
        alku_fonaw[32] = 16;

        for (a = 0; a < PaletteByteCount; a++)
        {
            textin[a] = (palette2[a] - palette[a]) * 256 / 64;
            textout[a] = (palette[a] - palette2[a]) * 256 / 64;
            picin[a] = (palette[a] - fade1[a]) * 256 / 128;
        }
    }

    void wait(int t)
    {
        for (int i = 0; i < t; i++)
        {
            if (demo_wantstoquit()) break;

            demo_vsync();
            demo_blit();
        }
    }

    void fonapois()
    {
        unsigned char * vmem = Shim::vram;

        for (unsigned int index = SCREEN_WIDTH * 64; index < 320U * (64 + 256); index++)
        {
            vmem[index] &= 63;
        }
    }

    void prt(int x, int y, const char * txt)
    {
        int x2 = 0;
        int y2w = y + 32;

        while (*txt)
        {
            int x2w = alku_fonaw[(unsigned char)*txt] + x;

            int sx = alku_fonap[(unsigned char)*txt];

            for (x2 = x; x2 < x2w; x2++)
            {
                for (int y2 = y; y2 < y2w; y2++)
                {
                    int d = font[(y2 - y) * FONT_STRIDE + sx];

                    Shim::vram[x2 + y2 * SCREEN_WIDTH] |= d;
                }
                sx++;
            }

            x = x2 + 2;

            txt++;
        }
    }

    void prtc(int x, int y, const char * txt)
    {
        int w = 0;
        const char * t = txt;

        while (*t)
            w += alku_fonaw[(unsigned char)*t++] + 2;

        prt(x - w / 2, y, txt);
    }

    void dofade(char * pal1, char * pal2)
    {
        static Palette pal;

        for (int index = 0; index < 64 && !demo_wantstoquit(); index++)
        {
            for (int b = 0; b < PaletteByteCount; b++)
                pal[b] = static_cast<char>(((pal1[b] * (64 - index) + pal2[b] * index) >> 6));

            Common::cop_pal = pal;

            Common::do_pal = 1;

            Common::copper2();
            Common::copper3();

            demo_vsync();
            demo_blit();
        }
    }

    int fdofade(char * pal1, char * pal2, int lerpValue)
    {
        if (lerpValue < 0 || lerpValue > 64) return 0;

        for (int b = 0; b < PaletteByteCount; b++)
            fuckpal[b] = static_cast<char>((pal1[b] * (64 - lerpValue) + pal2[b] * lerpValue) >> 6);

        Common::cop_pal = fuckpal;

        Common::do_pal = 1;

        return 0;
    }

    void addtext(int tx, int ty, const char * txt)
    {
        int w = 0;
        const char * t = txt;

        while (*t)
            w += alku_fonaw[(unsigned char)*t++] + 2;

        t = txt;
        w /= 2;
        while (*t)
        {
            for (int x = 0; x < alku_fonaw[(unsigned char)*t]; x++)
                for (int y = 0; y < 31; y++)
                    tbuf[y + ty][tx + x - w] = font[y * FONT_STRIDE + alku_fonap[(unsigned char)*t] + x];

            tx += alku_fonaw[(unsigned char)*t++] + 2;
        }
    }

    int alku_do_scroll(int mode)
    {
        if (mode != 0)
        {
            while (Common::frame_count < SCRLF)
            {
                Common::copper2();
                Common::copper3();

                demo_vsync();
                demo_blit();
            }
        }

        if (Common::frame_count < SCRLF) return 0;

        Common::frame_count -= SCRLF;

        if (mode == 1) ascrolltext(static_cast<short>(a), (int *)alku_dtau);

        Common::cop_start = a / 4;
        Common::cop_scrl = (a & 3);

        if ((a & 3) == 0)
        {
            outline(Data::hzpic + (a / 4 + 86) * 4 + 784, alku_planar_vram + 4 * ((a / 4 + 86) + 176 * 25));
            outline(Data::hzpic + (a / 4 + 86) * 4 + 784, alku_planar_vram + 4 * ((a / 4 + 86) + 176 * 25 + 88));
        }

        alku_simulate_scroll();

        a += 1;
        p ^= 1;

        return 1;
    }

    void faddtext(int tx, int ty, const char * txt)
    {
        int w = 0;
        const char * t = txt;

        while (*t)
            w += alku_fonaw[(unsigned char)*t++] + 2;

        t = txt;
        w /= 2;

        while (*t)
        {
            for (int x = 0; x < alku_fonaw[(unsigned char)*t]; x++)
                for (int y = 0; y < 32; y++)
                    tbuf[y + ty][tx + x - w] = font[y * FONT_STRIDE + alku_fonap[(unsigned char)*t] + x];

            alku_do_scroll(0);
            tx += alku_fonaw[(unsigned char)*t++] + 2;
        }
    }

    void fmaketext(int scrl)
    {
        unsigned char * vvmem = alku_planar_vram;
        short * p1 = alku_dtau;

        for (int y = 1; y < 184; y++)
            for (int x = SCREEN_WIDTH; x > 0; x--)
                if (tbuf[y][x] != tbuf[y][x - 1])
                {
                    *p1++ = static_cast<short>(x + y * 352);
                    *p1++ = tbuf[y][x] ^ tbuf[y][x - 1];
                }

        *p1++ = -1;
        *p1++ = -1;

        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            for (int y = 1; y < 184; y++)
            {
                vvmem[y * 352 + 352 * 100 + (x + scrl)] ^= tbuf[y][x];
            }
        }

        while (a <= scrl && !demo_wantstoquit())
        {
            Common::copper2();
            Common::copper3();

            alku_do_scroll(0);

            demo_vsync();
            demo_blit();
        }
    }

    void ffonapois()
    {
        unsigned int * vvmem = (unsigned int *)alku_planar_vram;

        for (unsigned int index = 80 * 64; index < 80U * (64 + 256 + 10); index++)
        {
            vvmem[index] = vvmem[index] & 0x3f3f3f3f;
        }

        alku_do_scroll(0);
    }

    void fffade(char * pal1, char * pal2, int frames)
    {
        for (int index = 0; index < PaletteByteCount; index++)
        {
            cfpal[index] = pal1[index];
            cfpal[index + PaletteByteCount] = static_cast<char>((pal2[index] - pal1[index]) * 256 / frames);
        }
    }

    void main()
    {
        a = {};
        p = {};
        alku_tptr = {};

        alku_init();

        while (Music::sync() < 1 && !demo_wantstoquit())
        {
            demo_blit();
        }

        if (demo_wantstoquit())
        {
            return;
        }

        prtc(160, 120, "A");
        prtc(160, 160, "Future Crew");
        prtc(160, 200, "Production");
        dofade(fade1, fade2);
        wait(300);
        dofade(fade2, fade1);
        fonapois();

        while (Music::sync() < 2 && !demo_wantstoquit())
        {
            demo_blit();
        }

        if (demo_wantstoquit())
        {
            return;
        }

        prtc(160, 160, "First Presented");
        prtc(160, 200, "at Assembly 93");
        dofade(fade1, fade2);
        wait(300);
        dofade(fade2, fade1);
        fonapois();

        while (Music::sync() < 3 && !demo_wantstoquit())
        {
            demo_blit();
        }

        if (demo_wantstoquit())
        {
            return;
        }

        prtc(160, 120, "in");
        prtc(160, 160, "\x8f");
        prtc(160, 179, "\x99");
        dofade(fade1, fade2);
        wait(300);
        dofade(fade2, fade1);
        fonapois();

        while (Music::sync() < 4 && !demo_wantstoquit())
        {
            demo_blit();
        }

        if (demo_wantstoquit())
        {
            return;
        }

        memcpy(Common::fadepal, fade1, PaletteByteCount);
        Common::cop_fadepal = picin;
        Common::cop_dofade = 128;

        int aa{}, f{};

        for (a = 1, p = 1, f = 0, Common::frame_count = 0; Common::cop_dofade != 0 && !demo_wantstoquit();)
        {
            Common::copper2();
            Common::copper3();

            alku_do_scroll(2);

            demo_vsync();
            demo_blit();
        }

        for (f = 60; a < SCREEN_WIDTH && !demo_wantstoquit();)
        {
            if (f == 0)
            {
                Common::cop_fadepal = textin;
                Common::cop_dofade = 64;
                f += 20;
            }
            else if (f == 50)
            {
                Common::cop_fadepal = textout;
                Common::cop_dofade = 64;
                f++;
            }
            else if (f > 50 && Common::cop_dofade == 0)
            {
                Common::cop_pal = palette;
                Common::do_pal = 1;
                f++;
                memset(tbuf, 0, 186 * SCREEN_WIDTH);
                switch (alku_tptr++)
                {
                    case 0:
                        addtext(160, 50, "Graphics");
                        addtext(160, 90, "Marvel");
                        addtext(160, 130, "Pixel");
                        ffonapois();
                        break;
                    case 1:
                        faddtext(160, 50, "Music");
                        faddtext(160, 90, "Purple Motion");
                        faddtext(160, 130, "Skaven");
                        ffonapois();
                        break;
                    case 2:
                        faddtext(160, 30, "Code");
                        faddtext(160, 70, "Psi");
                        faddtext(160, 110, "Trug");
                        faddtext(160, 148, "Wildfire");
                        ffonapois();
                        break;
                    case 3:
                        faddtext(160, 50, "Additional Design");
                        faddtext(160, 90, "Abyss");
                        faddtext(160, 130, "Gore");
                        ffonapois();
                        break;
                    case 4:
                        ffonapois();
                        break;
                    default:
                        faddtext(160, 80, "BUG BUG BUG");
                        faddtext(160, 130, "Timing error");
                        ffonapois();
                        break;
                }

                while (((a & 1) || Music::sync() < 4 + alku_tptr) && !demo_wantstoquit() && a < 319)
                {
                    Common::copper2();
                    Common::copper3();

                    alku_do_scroll(0);

                    demo_vsync();
                    demo_blit();
                }

                aa = a;
                if (aa < SCREEN_WIDTH - 12) fmaketext(aa + 16);
                f = 0;
            }
            else
                f++;

            Common::copper2();
            Common::copper3();

            alku_do_scroll(1);
            demo_vsync();
            demo_blit();
        }

        alku_do_scroll(1);
        demo_blit();

        if (f > 63 / SCRLF)
        {
            dofade(palette2, palette);
        }

        fonapois();
    }
}
