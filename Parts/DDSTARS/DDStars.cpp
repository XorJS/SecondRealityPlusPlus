#include "Parts/Common.h"

#include "DDStars_Data.h"

namespace DDStars
{
    constexpr const size_t STARS = 512;
    constexpr const size_t STARS2 = 1024;
    constexpr const size_t PLANE_SIZE = PLANAR_WIDTH * DOUBlE_SCREEN_HEIGHT;
    constexpr const size_t PAGESIZE = 16 * 1024;

    namespace
    {
        uint16_t emmpage4 = 0;

        uint16_t starlimit = 0;
        int16_t startxtopen = 0;
        int16_t startxtclose = 0;
        uint16_t startxtp0 = 0;
        uint16_t starframe = 0;
        uint8_t starpalfade = 0;
        uint16_t _nostar1 = 200;
        uint16_t _nostar2 = 199;

        uint16_t rows1[200]{};
        uint16_t rows[DOUBlE_SCREEN_HEIGHT]{};

        uint16_t star[STARS2 * 4]{}; // age, xseed, yseed, pad (words)

        uint16_t muldivx[256]{};
        uint16_t muldivy[256]{};

        uint32_t seed = 0;

        uint8_t emmdata[32 * 2 * PAGESIZE]{};

        uint8_t * curemmptr0 = nullptr;
        uint8_t * curemmptr1 = nullptr;

        uint8_t ddstars_planar[PLANE_SIZE * 4]{};
    }

    static inline uint8_t sat_lshift3_inc(uint8_t in)
    {
        uint16_t s = ((uint16_t)(in + 1)) << 3;

        return (s > 255) ? 255u : (uint8_t)s;
    }

    static inline uint16_t rng16()
    {
        uint32_t a = 0x343FDu;
        uint64_t prod = (uint64_t)a * (uint64_t)seed;
        seed = (uint32_t)((uint32_t)prod + 0x269EC3u);

        return (uint16_t)((prod >> 32) & 0xFFFFu);
    }

    static inline uint8_t * fetch4ax(uint16_t ax)
    {
        return emmdata + ((uint32_t)ax << 13);
    }

    static inline uint8_t * fetch4ax2(uint16_t ax)
    {
        return emmdata + ((uint32_t)ax << 13) + 0x1000;
    }

    static void resolve_planar()
    {
        uint8_t * dst = Shim::vram;
        uint8_t * src0 = emmdata + 0;

        src0 = ddstars_planar;

        const uint8_t * p0 = src0 + PLANE_SIZE * 0;
        const uint8_t * p1 = src0 + PLANE_SIZE * 1;
        const uint8_t * p2 = src0 + PLANE_SIZE * 2;
        const uint8_t * p3 = src0 + PLANE_SIZE * 3;

        size_t byteIndex = 0;
        uint8_t bl = 0x80;

        for (uint32_t i = 0; i < 320u * 400u; ++i)
        {
            uint8_t al = 0;

            if (p0[byteIndex] & bl) al |= 1;
            if (p1[byteIndex] & bl) al |= 2;
            if (p2[byteIndex] & bl) al |= 4;
            if (p3[byteIndex] & bl) al |= 8;

            *dst++ = al;

            bl = (uint8_t)((bl >> 1) | (bl << 7));

            if (bl & 0x80) ++byteIndex;
        }
    }

    static void clearsbu()
    {
        for (uint32_t ax = 0, i = 0; i < 128; ++i, ++ax)
        {
            uint8_t * edi = fetch4ax((uint16_t)ax);
            memset(edi, 0, 4096u * 2u);
        }
    }

    static void starfetch0()
    {
        uint16_t ax = (uint16_t)((emmpage4 + 1u) & 63u);
        emmpage4 = ax;
        curemmptr0 = fetch4ax(ax);
    }

    static void starfetch1()
    {
        uint16_t ax = (uint16_t)((emmpage4 + 17u) & 63u);
        curemmptr1 = fetch4ax2(ax);
    }

    // Star plotter core (for STARS batch).
    static void staradd()
    {
        // Decrement starlimit once if nonzero
        if (starlimit) --starlimit;
        int bp = STARS;
        uint16_t * s = star;

        while (bp--)
        {
            // age-=2 (byte); if underflow -> reseed this star
            uint8_t age = (uint8_t)(s[0] & 0x00FF);
            age -= 2;

            s[0] = (uint16_t)((s[0] & 0xFF00u) | age);

            if (age > 0xF0)
            { // carry set -> underflow
                s[1] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
                s[2] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
                s += 4;

                continue;
            }

            if ((unsigned)bp < (unsigned)starlimit)
            {
                s += 4;
                continue;
            }

            uint32_t idx = (uint32_t)age;

            int32_t prodY = (int32_t)(int16_t)s[2] * (int32_t)(int16_t)muldivy[idx];
            int32_t y = (prodY >> 14) + 100;

            if ((uint32_t)y > 99u)
            {
                s += 4;
                continue;
            }

            int32_t prodX = (int32_t)(int16_t)s[1] * (int32_t)(int16_t)muldivx[idx];
            int32_t x = (prodX >> 14) + 160;

            if ((uint32_t)x > 319u)
            {
                s += 4;
                continue;
            }

            uint32_t byteOff = rows1[(uint16_t)y] + (uint32_t)(x >> 3);
            uint8_t mask = (uint8_t)(0x80u >> (x & 7));

            uint8_t a = (uint8_t)age;

            // Plane selection based on age threshold
            if (a >= 180)
            {
                curemmptr0[byteOff] |= mask;
            }
            else if (a >= 110)
            {
                curemmptr0[byteOff + 4096] |= mask;
            }
            else
            {
                curemmptr0[byteOff] |= mask;
                curemmptr0[byteOff + 4096] |= mask;
            }

            s += 4;
        }
    }

    // init_stars: clear, build row tables, seed stars, build mul tables, prime palette state
    static void init_stars()
    {
        CLEAR(ddstars_planar);

        clearsbu();

        {
            uint16_t acc = 0;
            for (int i = 0; i < SCREEN_HEIGHT; ++i)
            {
                rows1[i] = acc;
                acc += (uint16_t)(SCREEN_WIDTH / 8);
            }
        }

        {
            uint16_t acc = 0;
            for (int i = 0; i < DOUBlE_SCREEN_HEIGHT; ++i)
            {
                rows[i] = acc;
                acc += (uint16_t)(SCREEN_WIDTH / 8);
            }
        }

        // Seed star table: (age=i-1), xseed=rnd[-512..+511], yseed=rnd[-512..+511], pad=0
        for (int i = 0; i < static_cast<int>(STARS2); ++i)
        {
            uint16_t age = (uint16_t)((i & 0xFF) ? (i & 0xFF) - 1 : 0);

            star[i * 4 + 0] = age;
            star[i * 4 + 1] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
            star[i * 4 + 2] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
            star[i * 4 + 3] = 0;
        }

        {
            int t = 150;
            for (int k = 0; k < 256; ++k, t += 4)
                muldivy[k] = (uint16_t)(((uint32_t)((108u << 16) / t)) >> 1);
        }

        {
            int t = 150;
            for (int k = 0; k < 256; ++k, t += 4)
                muldivx[k] = (uint16_t)(((uint32_t)((144u << 16) / t)) >> 1);
        }

        starlimit = STARS;
        starpalfade = 0;
        startxtopen = -9999;
        startxtclose = 10000;

        // Pre-warm with 100 star batches
        for (int c = 0; c < 100; ++c)
        {
            starfetch0();
            staradd();
        }
    }

    static void staradd2()
    {
        starfetch1();
        if (starlimit) starlimit = (uint16_t)(starlimit - 4);
        int bp = STARS2;
        uint16_t * s = star;

        while (bp--)
        {
            uint8_t age = (uint8_t)(s[0] & 0x00FF);
            age -= 2;
            s[0] = (uint16_t)((s[0] & 0xFF00u) | age);

            if (age > 0xF0)
            {
                s[1] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
                s[2] = (uint16_t)((int16_t)((rng16() & 1023) - 512));
                s += 4;
                continue;
            }

            if ((unsigned)bp < (unsigned)starlimit)
            {
                s += 4;
                continue;
            }

            uint32_t idx = (uint32_t)age;

            int32_t y = (((int32_t)(int16_t)s[2] * (int32_t)(int16_t)muldivy[idx]) >> 14) + 100;
            if ((uint32_t)y > 99u)
            {
                s += 4;
                continue;
            }

            int32_t x = (((int32_t)(int16_t)s[1] * (int32_t)(int16_t)muldivx[idx]) >> 14) + 160;
            if ((uint32_t)x > 319u)
            {
                s += 4;
                continue;
            }

            uint32_t byteOff = rows1[(uint16_t)y] + (uint32_t)(x >> 3);
            uint8_t mask = (uint8_t)(0x80u >> (x & 7));

            if (uint8_t a = (uint8_t)age; a >= 180)
            {
                curemmptr0[byteOff] |= mask;
                curemmptr1[byteOff] |= mask;
            }
            else if (a >= 110)
            {
                curemmptr0[byteOff + 4096] |= mask;
                curemmptr1[byteOff + 4096] |= mask;
            }
            else
            {
                curemmptr0[byteOff] |= mask;
                curemmptr0[byteOff + 4096] |= mask;
                curemmptr1[byteOff] |= mask;
                curemmptr1[byteOff + 4096] |= mask;
            }

            s += 4;
        }
    }

    // risetext: exact compositing and edge clearing behavior into planar buffer
    static void risetext()
    {
        // open/close integrators
        int16_t ax = startxtopen;
        if (ax < 99)
        {
            ++ax;
            startxtopen = ax;
        }
        int16_t dx = startxtclose;
        if (dx > 0)
        {
            --dx;
            startxtclose = dx;
        }

        if (dx < ax) ax = dx;

        if (ax <= 0) return;

        if (ax <= 1) ax = 2; // clamp early ramp

        int use = ax;

        _nostar2 = (uint16_t)use;
        uint32_t ediOff = (uint32_t)(150 - use) * 80u;
        _nostar1 = (uint16_t)ediOff;

        uint8_t * edi = ddstars_planar + ediOff;
        const uint8_t * esi = Data::texts_16 + 0x40 + startxtp0;

        int ecx = use - 1;

        edi -= 40;
        memset(edi + PLANE_SIZE * 0, 0, 40);
        memset(edi + PLANE_SIZE * 1, 0, 40);
        memset(edi + PLANE_SIZE * 2, 0, 40);
        memset(edi + PLANE_SIZE * 3, 0, 40);
        edi += PLANAR_WIDTH;
        if (--ecx == 0)
        {
            memset(edi, 0, PLANAR_WIDTH);
            return;
        }

        // Second clear on all planes
        memset(edi + PLANE_SIZE * 0, 0, 40);
        memset(edi + PLANE_SIZE * 1, 0, 40);
        memset(edi + PLANE_SIZE * 2, 0, 40);
        memset(edi + PLANE_SIZE * 3, 0, 40);

        edi += PLANAR_WIDTH;

        if (--ecx == 0)
        {
            return;
        }

        if (--ecx == 0)
        {
            memset(edi, 0, 40);
            edi += PLANAR_WIDTH;
            _nostar2 = (uint16_t)((uintptr_t)edi - (uintptr_t)ddstars_planar);
            return;
        }

        // Main copy: rows come in pairs: first -> planes 0&2, second -> planes 1&2
        for (;;)
        {
            // top of pair
            memcpy(edi + PLANE_SIZE * 0, esi, 40);
            memcpy(edi + PLANE_SIZE * 2, esi, 40);
            esi += 40;

            // bottom of pair
            memcpy(edi + PLANE_SIZE * 1, esi, 40);
            memcpy(edi + PLANE_SIZE * 2, esi, 40);
            esi += 40;

            edi += PLANAR_WIDTH;
            if (--ecx == 0)
            {
                memset(edi, 0, 40);
                edi += PLANAR_WIDTH;
                _nostar2 = (uint16_t)((uintptr_t)edi - (uintptr_t)ddstars_planar);
                return;
            }
            if (ecx <= 0) break;
        }
    }

    static void do_stars()
    {
        for (;;)
        {
            demo_vsync();

            // Palette ramp (exact arithmetic & saturation)
            if (starpalfade <= 32)
            {
                uint8_t bl = sat_lshift3_inc(starpalfade);
                starpalfade = (uint8_t)(starpalfade + 1);

                // Set DAC index 0
                Shim::outp(0x3C8, 0);

                // Black (first color)
                Shim::outp(0x3C9, 0);
                Shim::outp(0x3C9, 0);
                Shim::outp(0x3C9, 0);

                Shim::outp(0x3C9, (uint8_t)(((25 * 70 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((31 * 70 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((38 * 70 / 100) * bl) >> 8));

                Shim::outp(0x3C9, (uint8_t)(((45 * 56 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((58 * 56 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((69 * 56 / 100) * bl) >> 8));

                Shim::outp(0x3C9, (uint8_t)(((67 * 64 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((84 * 64 / 100) * bl) >> 8));
                Shim::outp(0x3C9, (uint8_t)(((99 * 64 / 100) * bl) >> 8));

                Shim::outp(0x3C9, 0);
                Shim::outp(0x3C9, 0);
                Shim::outp(0x3C9, 0);
                Shim::outp(0x3C9, 10);
                Shim::outp(0x3C9, 20);
                Shim::outp(0x3C9, 35);
                Shim::outp(0x3C9, 20);
                Shim::outp(0x3C9, 30);
                Shim::outp(0x3C9, 45);
                Shim::outp(0x3C9, 30);
                Shim::outp(0x3C9, 40);
                Shim::outp(0x3C9, 60);
            }

            // Timers / triggers
            if (++starframe; starframe == 1200)
            {
                startxtp0 = 80;
                startxtopen = -256;
                startxtclose = 1500;
            }

            if (starframe == 3200)
            {
                startxtp0 = 101 * 80;
                startxtopen = -256;
                startxtclose = 1500;
            }

            if (starframe == 1500)
            {
                starlimit = STARS2;
            }

            // Integrate pages & draw stars
            starfetch0();

            if (starframe > 1200)
            {
                staradd2();
            }
            else if (starframe > 900)
            { /* no add */
            }
            else
            {
                staradd();
            }

            // Copy top halves (100 rows) from current page into planes 0 and 1
            {
                uint8_t * src = curemmptr0;
                uint8_t * dst = ddstars_planar + PLANE_SIZE * 0;
                for (int r = 0; r < 100; ++r, src += 40, dst += PLANAR_WIDTH)
                    memcpy(dst, src, 40);
            }

            {
                uint8_t * src = curemmptr0 + 4096;
                uint8_t * dst = ddstars_planar + PLANE_SIZE * 1;
                for (int r = 0; r < 100; ++r, src += 40, dst += PLANAR_WIDTH)
                    memcpy(dst, src, 40);
            }

            // Bottom halves pulled from (emmpage4+32)&63, reversed rows
            {
                uint16_t ax = (uint16_t)((emmpage4 + 32u) & 63u);
                uint8_t * base = fetch4ax(ax);
                // plane 0 bottom
                uint8_t * src = base + 99 * 40;
                uint8_t * dst = ddstars_planar + PLANE_SIZE * 0 + 200 * 40;

                for (int r = 0; r < 100; ++r, src -= 40, dst += PLANAR_WIDTH)
                    memcpy(dst, src, 40);

                // plane 1 bottom
                src = base + 99 * 40 + 4096;
                dst = ddstars_planar + PLANE_SIZE * 1 + 200 * 40;

                for (int r = 0; r < 100; ++r, src -= 40, dst += PLANAR_WIDTH)
                    memcpy(dst, src, 40);
            }

            risetext();

            resolve_planar();

            demo_blit();

            if (demo_wantstoquit()) break;
        }
    }

    void main()
    {
        Shim::clearScreen();

        init_stars();
        do_stars();
    }
}
