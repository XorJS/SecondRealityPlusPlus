#include "Parts/Common.h"

namespace Dots
{
    struct Dot
    {
        short x;
        short y;
        short z;
        short old1;
        short old2;
        short old3;
        short old4;
        short yadd;
    };

    namespace
    {
        short bpmin = 30000;
        short bpmax = -30000;
        short dotnum = 0;

        short rotsin = 0;
        short rotcos = 0;

        char bgpic[SCREEN_SIZE]{};

        short dots_rows[200]{};

        int32_t depthtable1[128]{};
        int32_t depthtable2[128]{};
        int32_t depthtable3[128]{};
        int32_t depthtable4[128]{};

        Palette pal{};
        Palette pal2{};

        Dot dot[1024];

        short gravitybottom = 8000;
        short gravity = 0;
        short gravityd = 16;

        int dottaul[1024]{};

        const int cols[] = { 0, 0, 0, 4, 25, 30, 8, 40, 45, 16, 55, 60 };

        int rand_seed = 10;
    }

    int rand()
    {
        rand_seed = rand_seed * 1103515245 + 12345;

        return (unsigned int)(rand_seed / 65536) & 32767;
    }

    void setpalette(char * p)
    {
        Shim::outp(0x3c8, 0);

        for (int c = 0; c < PaletteByteCount; c++)
            Shim::outp(0x3c9, p[c]);
    }

    static inline int isin(int deg)
    {
        return (Common::Data::sin1024[deg & 1023]);
    }

    static inline int icos(int deg)
    {
        return (Common::Data::sin1024[(deg + 256) & 1023]);
    }

    static inline void restore_ball(uint16_t pos)
    {
        memcpy(&Shim::vram[pos], &bgpic[pos], 4);
        memcpy(&Shim::vram[pos + SCREEN_WIDTH], &bgpic[pos + SCREEN_WIDTH], 4);
        memcpy(&Shim::vram[pos + 640], &bgpic[pos + 640], 4);
    }

    static inline void restore_shadow(uint16_t pos)
    {
        memcpy(&Shim::vram[pos], &bgpic[pos], 2);
    }

    static inline void draw_shadow(uint16_t pos)
    {
        Shim::vram[pos + 0] = 87;
        Shim::vram[pos + 1] = 87;
    }

    static inline void draw_ball_from_depth(uint16_t pos, int idx)
    {
        *(uint16_t *)(&Shim::vram[pos + 1]) = static_cast<uint16_t>(depthtable1[idx]);
        *(uint32_t *)(&Shim::vram[pos + SCREEN_WIDTH]) = depthtable2[idx];
        *(uint16_t *)(&Shim::vram[pos + 641]) = static_cast<uint16_t>(depthtable3[idx]);
    }

    void drawdots()
    {
        for (uint16_t k = 0; k < dotnum; ++k)
        {
            Dot * currentDot = &dot[k];

            int32_t x = currentDot->x;
            int32_t z = currentDot->z;

            int32_t t1 = (int32_t)z * (int32_t)rotcos;
            int32_t t2 = (int32_t)x * (int32_t)rotsin;
            int32_t depth32 = t1 - t2;
            int32_t bp = (depth32 >> 16);
            bp += 9000;
            if (bp == 0) bp = 1;

            int32_t sum32 = (int32_t)x * (int32_t)rotcos + (int32_t)z * (int32_t)rotsin;
            int32_t xnum = (sum32 >> 8) + (sum32 >> 11);
            int sx = (int)(xnum / bp) + 160;

            if ((unsigned)sx > 319u)
            {
                uint16_t oldPos = (uint16_t)(uint16_t)currentDot->old2;
                restore_ball(oldPos);
                uint16_t oldSh = (uint16_t)(uint16_t)currentDot->old1;
                restore_shadow(oldSh);
                continue;
            }

            if (int sy_shadow = (int)(((int32_t)8 << 16) / bp) + 100; (unsigned)sy_shadow <= 199u)
            {
                uint16_t rowOff = (uint16_t)dots_rows[sy_shadow];
                uint16_t shPos = (uint16_t)(rowOff + sx);

                uint16_t prevSh = (uint16_t)currentDot->old1;
                restore_shadow(prevSh);

                draw_shadow(shPos);

                currentDot->old1 = (int16_t)shPos;
            }
            else
            {
                restore_shadow((uint16_t)currentDot->old1);
            }

            currentDot->yadd = (int16_t)(currentDot->yadd + gravity);

            int32_t yNew = (int32_t)currentDot->y + (int32_t)currentDot->yadd;

            if (yNew >= gravitybottom)
            {
                int32_t v = currentDot->yadd;
                v = -v;
                v = (v * (int32_t)gravityd) >> 4;
                currentDot->yadd = (int16_t)v;
                yNew += v;
            }

            currentDot->y = (int16_t)yNew;

            int32_t ynum = ((int32_t)currentDot->y << 6) / bp;
            int sy = (int)ynum + 100;

            if ((unsigned)sy <= 199u)
            {
                uint16_t rowOff = (uint16_t)dots_rows[sy];
                uint16_t pos = (uint16_t)(rowOff + sx);

                uint16_t oldPos = (uint16_t)currentDot->old2;
                restore_ball(oldPos);

                int idx = (int)((bp >> 6) & ~3);
                if (idx < 0) idx = 0;
                if (idx > 127) idx = 127;

                draw_ball_from_depth(pos, idx);

                currentDot->old2 = (int16_t)pos;
            }
            else
            {
                restore_ball((uint16_t)currentDot->old2);
            }
        }
    }

    void main()
    {
        CLEAR(bgpic);
        CLEAR(dots_rows);

        CLEAR(depthtable1);
        CLEAR(depthtable2);
        CLEAR(depthtable3);
        CLEAR(depthtable4);

        bpmin = 30000;
        bpmax = -30000;
        dotnum = 0;
        rotsin = 0;
        rotcos = 0;

        gravitybottom = 8000;
        gravity = 0;
        gravityd = 16;

        int dropper, repeat;
        int frame = 0;
        int rota = -1 * 64;
        int rot = 0, rots = 0;
        int a, b, c, d, i, j = 0;
        int grav, gravd;
        int f = 0;

        Shim::clearScreen();

        dotnum = 512;

        for (a = 0; a < dotnum; a++)
            dottaul[a] = a;

        for (a = 0; a < 500; a++)
        {
            b = rand() % dotnum;
            c = rand() % dotnum;
            d = dottaul[b];
            dottaul[b] = dottaul[c];
            dottaul[c] = d;
        }

        {
            dropper = 22000;

            for (a = 0; a < dotnum; a++)
            {
                dot[a].x = 0;
                dot[a].y = static_cast<short>(2560 - dropper);
                dot[a].z = 0;
                dot[a].yadd = 0;
            }

            grav = 3;
            gravd = 13;
            gravitybottom = 8105;
            i = -1;
        }

        for (a = 0; a < 500; a++)
        { // scramble
            b = rand() % dotnum;
            c = rand() % dotnum;
            d = dot[b].x;
            dot[b].x = dot[c].x;
            dot[c].x = static_cast<short>(d);
            d = dot[b].y;
            dot[b].y = dot[c].y;
            dot[c].y = static_cast<short>(d);
            d = dot[b].z;
            dot[b].z = dot[c].z;
            dot[c].z = static_cast<short>(d);
        }

        for (a = 0; a < SCREEN_HEIGHT; a++)
            dots_rows[a] = static_cast<short>(a * SCREEN_WIDTH);

        Shim::outp(0x3c8, 0);

        for (a = 0; a < 16; a++)
            for (b = 0; b < 4; b++)
            {
                c = 100 + a * 9;
                Shim::outp(0x3c9, cols[b * 3 + 0]);
                Shim::outp(0x3c9, cols[b * 3 + 1] * c / 256);
                Shim::outp(0x3c9, cols[b * 3 + 2] * c / 256);
            }

        Shim::outp(0x3c8, 255);
        Shim::outp(0x3c9, 31);
        Shim::outp(0x3c9, 0);
        Shim::outp(0x3c9, 15);
        Shim::outp(0x3c8, 64);

        for (a = 0; a < 100; a++)
        {
            c = 64 - 256 / (a + 4);
            c = c * c / 64;
            Shim::outp(0x3c9, c / 4);
            Shim::outp(0x3c9, c / 4);
            Shim::outp(0x3c9, c / 4);
        }

        Shim::outp(0x3c7, 0);

        for (a = 0; a < PaletteByteCount; a++)
            pal[a] = Shim::inp(0x3c9);

        Shim::outp(0x3c8, 0);

        for (a = 0; a < PaletteByteCount; a++)
            Shim::outp(0x3c9, 0);

        for (a = 0; a < 100; a++)
        {
            memset(Shim::vram + (100 + a) * SCREEN_WIDTH, a + 64, SCREEN_WIDTH);
        }

        for (a = 0; a < 128; a++)
        {
            c = a - (43 + 20) / 2;
            c = c * 3 / 4;
            c += 8;
            if (c < 0)
                c = 0;
            else if (c > 15)
                c = 15;
            c = 15 - c;
            depthtable1[a] = 0x202 + 0x04040404 * c;
            depthtable2[a] = 0x02030302 + 0x04040404 * c;
            depthtable3[a] = 0x202 + 0x04040404 * c;
        }

        memcpy(bgpic, Shim::vram, SCREEN_SIZE);

        a = 0;
        for (b = 64; b >= 0; b--)
        {
            for (c = 0; c < PaletteByteCount; c++)
            {
                a = pal[c] - b;
                if (a < 0) a = 0;
                pal2[c] = static_cast<char>(a);
            }

            demo_vsync();
            demo_vsync();

            Shim::outp(0x3c8, 0);

            for (c = 0; c < PaletteByteCount; c++)
                Shim::outp(0x3c9, pal2[c]);

            demo_blit();
        }

        while (!demo_wantstoquit() && frame < 2450)
        {
            repeat = demo_vsync();

            if (frame > 2300) setpalette(pal2);

            {
                a = Music::getPlusFlags();
                if (a > -4 && a < 0) break;
            }

            while (repeat--)
            {
                frame++;

                if (frame == 500) f = 0;
                i = dottaul[j];
                j++;
                j %= dotnum;

                if (frame < 500)
                {
                    dot[i].x = static_cast<short>(isin(f * 11) * 40);
                    dot[i].y = static_cast<short>(icos(f * 13) * 10 - dropper);
                    dot[i].z = static_cast<short>(isin(f * 17) * 40);
                    dot[i].yadd = 0;
                }
                else if (frame < 900)
                {
                    dot[i].x = static_cast<short>(icos(f * 15) * 55);
                    dot[i].y = static_cast<short>(dropper);
                    dot[i].z = static_cast<short>(isin(f * 15) * 55);
                    dot[i].yadd = -260;
                }
                else if (frame < 1700)
                {
                    a = Common::Data::sin1024[frame & 1023] / 8;
                    dot[i].x = static_cast<short>(icos(f * 66) * a);
                    dot[i].y = 8000;
                    dot[i].z = static_cast<short>(isin(f * 66) * a);
                    dot[i].yadd = -300;
                }
                else if (frame < 2360)
                {
                    dot[i].x = static_cast<short>(rand() - 16384);
                    dot[i].y = static_cast<short>(8000 - rand() / 2);
                    dot[i].z = static_cast<short>(rand() - 16384);
                    dot[i].yadd = 0;
                    if (frame > 1900 && !(frame & 31) && grav > 0) grav--;
                }
                else if (frame < 2400)
                {
                    a = frame - 2360;
                    for (b = 0; b < PaletteByteCount; b += 3)
                    {
                        c = pal[b + 0] + a * 3;
                        if (c > 63) c = 63;
                        pal2[b + 0] = static_cast<char>(c);
                        c = pal[b + 1] + a * 3;
                        if (c > 63) c = 63;
                        pal2[b + 1] = static_cast<char>(c);
                        c = pal[b + 2] + a * 4;
                        if (c > 63) c = 63;
                        pal2[b + 2] = static_cast<char>(c);
                    }
                }
                else if (frame < 2440)
                {
                    a = frame - 2400;
                    for (b = 0; b < PaletteByteCount; b += 3)
                    {
                        c = 63 - a * 2;
                        if (c < 0) c = 0;
                        pal2[b + 0] = static_cast<char>(c);
                        pal2[b + 1] = static_cast<char>(c);
                        pal2[b + 2] = static_cast<char>(c);
                    }
                }

                if (dropper > 4000) dropper -= 100;
                rotcos = static_cast<short>(icos(rot) * 64);
                rotsin = static_cast<short>(isin(rot) * 64);
                rots += 2;

                if (frame > 1900)
                {
                    rot += rota / 64;
                    rota--;
                }
                else
                    rot = isin(rots);

                f++;
                gravity = static_cast<short>(grav);
                gravityd = static_cast<short>(gravd);
            }

            drawdots();

            demo_blit();
        }
    }
}
