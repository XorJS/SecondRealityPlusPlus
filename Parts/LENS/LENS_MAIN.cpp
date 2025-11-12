#include "Parts/Common.h"

#include "LENS_MAIN_Data.h"

#define DOWORD(dst_offset, src_offset1, src_offset2, src_base, offset_table, mask, dst_base) \
    do                                                                                       \
    {                                                                                        \
        int16_t idx1 = *(int16_t *)&offset_table[src_offset1];                               \
        uint8_t al = src_base[idx1];                                                         \
        int16_t idx2 = *(int16_t *)&offset_table[src_offset2];                               \
        uint8_t ah = src_base[idx2];                                                         \
        uint16_t ax = al | (ah << 8) | mask;                                                 \
        *(uint16_t *)&dst_base[dst_offset] = ax;                                             \
    } while (0)

namespace Lens
{
    namespace
    {
        constexpr const size_t ZOOMXW = SCREEN_WIDTH / 2;
        constexpr const size_t ZOOMYW = SCREEN_HEIGHT / 2;

        short * pathdata1 = nullptr;
        short * pathdata2 = nullptr;
        uint8_t * back = nullptr;
        uint8_t * rotpic = nullptr;
        uint8_t * rotpic90 = nullptr;

        int lenswid{}, lenshig{}, lensxs{}, lensys{};
        int *lens1{}, *lens2{}, *lens3{}, *lens4{};

        Palette palette{};

        char lensexb[64784 + 4096]{};
        char pathdata[13000]{};
        char lens_fade[16000]{};
        char lens_fade2[20000]{};

        char zoomer_planar_buffer[SCREEN_WIDTH * SCREEN_HEIGHT]{};

        uint8_t _rotpic[16384 * 4]{};

        int firfade1[200]{};
        int firfade2[200]{};
        int firfade1a[200]{};
        int firfade2a[200]{};
    }

    void dorowC(int * lens, int U, int Y, int M)
    {
        uint8_t * esi;
        uint8_t * edi;
        uint8_t * ebp;
        uint16_t mask;
        int16_t count;

        mask = (M & 0xFF) | ((M & 0xFF) << 8);

        esi = (uint8_t *)lens + (Y << 2);
        count = *(int16_t *)(esi + 2);
        uint16_t offset = *(uint16_t *)esi;

        if (count < 4) return;

        // Get offset table
        esi = (uint8_t *)lens + offset;
        int16_t displacement = *(int16_t *)esi;

        // Setup pointers
        edi = (uint8_t *)back + U + displacement;
        ebp = Shim::vram + U + displacement;
        esi += 2;

        // Handle odd alignment
        if ((uintptr_t)ebp & 1)
        {
            int16_t idx = *(int16_t *)esi;
            esi += 2;
            *ebp = edi[idx] | (M & 0xFF);
            ebp++;
            count--;
        }

        // Process pairs
        int pairs = count >> 1;
        esi -= SCREEN_WIDTH;
        ebp -= SCREEN_WIDTH;

        int start = (64 - pairs);
        if (start < 0) start = 0;

        for (int i = start; i < 64 && i < start + pairs; i++)
        {
            int idx = 63 - i;
            DOWORD(SCREEN_WIDTH + idx * 2, SCREEN_WIDTH + idx * 4, SCREEN_WIDTH + idx * 4 + 2, edi, esi, mask, ebp);
        }

        // Handle remaining odd byte
        if (count & 1)
        {
            ebp += count & ~1;
            esi += (count & ~1) << 1;
            int16_t idx = *(int16_t *)(esi + SCREEN_WIDTH);
            ebp[SCREEN_WIDTH] = edi[idx] | (M & 0xFF);
        }
    }

    void dorow2C(int * lens, int U, int Y, int M)
    {
        uint8_t * esi;
        uint8_t * edi;
        uint8_t * ebp;
        uint8_t mask;
        int16_t count;

        mask = M & 0xFF;

        // Get row info
        esi = (uint8_t *)lens + (Y << 2);
        count = *(int16_t *)(esi + 2);
        uint16_t offset = *(uint16_t *)esi;

        if (!count) return;

        // Get base offset
        esi = (uint8_t *)lens + offset;
        int16_t base = *(int16_t *)esi;

        // Setup pointers
        edi = (uint8_t *)back + U + base;
        ebp = (uint8_t *)lens + offset + 2;

        // Process remapped pixels
        while (count--)
        {
            int16_t src_idx = *(int16_t *)(ebp + 2);
            int16_t dst_idx = *(int16_t *)ebp;
            Shim::vram[U + base + dst_idx] = edi[src_idx] | mask;
            ebp += 4;
        }
    }

    static inline int16_t read_le_i16(const void * p)
    {
        const uint8_t * b = (const uint8_t *)p;
        return (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    }

    static inline uint16_t read_le_u16(const void * p)
    {
        const uint8_t * b = (const uint8_t *)p;
        return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    }

    void dorow3C(int * ptr, int U, int Y)
    {
        const uint8_t * base = (const uint8_t *)ptr;
        const uint8_t * rowDesc = base + ((uint32_t)Y << 2);

        uint16_t count = read_le_u16(rowDesc + 2);

        if (count == 0)
        {
            return;
        }

        uint16_t offsToRowTable = read_le_u16(rowDesc + 0);
        const uint8_t * rowTab = base + offsToRowTable;

        int baseIndex = U + (int)read_le_i16(rowTab);

        rowTab += 2;

        for (uint16_t i = 0; i < count; ++i)
        {
            int rel = (int)read_le_i16(rowTab);

            rowTab += 2;

            int idx = baseIndex + rel;

            Shim::vram[idx] = back[idx];
        }
    }

    static inline int32_t asm_style_scale307(int32_t v)
    {
        uint32_t lo = (uint32_t)v * 307u;
        return (int32_t)lo >> 8;
    }

    void rotateC(int x, int y, int xa, int ya)
    {
        int32_t xpos = (int32_t)x << 16;
        int32_t ypos = (int32_t)y << 16;

        int32_t Xadd = ((int32_t)(int16_t)ya) << 6;
        int32_t Yadd = ((int32_t)(int16_t)xa) << 6;

        char * src = (char *)rotpic;
        {
            int32_t ecx = Xadd;
            if (ecx < 0) ecx = -ecx;
            int32_t edx = Yadd;
            if (edx < 0) edx = -edx;
            if (ecx > edx)
            {
                // rotpic90, swap/neg gradients, swap/neg position like ASM
                src = (char *)rotpic90;

                int32_t t = Xadd;
                Xadd = -Yadd;
                Yadd = t;

                int32_t tx = xpos, ty = ypos;
                xpos = -ty;
                ypos = tx;
            }
        }

        uint16_t moda_lo[ZOOMXW / 4], moda_hi[ZOOMXW / 4];
        uint16_t modb_lo[ZOOMXW / 4], modb_hi[ZOOMXW / 4];

        {
            uint16_t si = 0, di = 0; // low accumulators
            uint8_t al = 0, ah = 0;  // high-byte accumulators

            uint16_t cx = (uint16_t)(Yadd & 0xFFFF);      // [yadd+0]
            uint16_t dx = (uint16_t)(Xadd & 0xFFFF);      // [xadd+0]
            uint8_t bl = (uint8_t)((uint32_t)Yadd >> 16); // [yadd+2]
            uint8_t bh = (uint8_t)((uint32_t)Xadd >> 16); // [xadd+2]

            bh = (uint8_t)(-(int8_t)bh);
            {
                uint16_t olddx = dx;

                dx = (uint16_t)(0u - dx);

                if (olddx != 0)
                {
                    bh = (uint8_t)(bh - 1u);
                }
            }

            for (int i = 0; i < static_cast<int>(ZOOMXW / 4); ++i)
            {
                {
                    uint32_t t = (uint32_t)si + (uint32_t)cx;
                    si = (uint16_t)t;
                    uint8_t c = (uint8_t)(t >> 16);
                    al = (uint8_t)(al + bl + c);

                    t = (uint32_t)di + (uint32_t)dx;
                    di = (uint16_t)t;
                    c = (uint8_t)(t >> 16);
                    ah = (uint8_t)(ah + bh + c);

                    moda_lo[i] = (uint16_t)((uint16_t)al | ((uint16_t)ah << 8));
                }

                {
                    uint32_t t = (uint32_t)si + (uint32_t)cx;
                    si = (uint16_t)t;
                    uint8_t c = (uint8_t)(t >> 16);
                    al = (uint8_t)(al + bl + c);

                    t = (uint32_t)di + (uint32_t)dx;
                    di = (uint16_t)t;
                    c = (uint8_t)(t >> 16);
                    ah = (uint8_t)(ah + bh + c);

                    modb_lo[i] = (uint16_t)((uint16_t)al | ((uint16_t)ah << 8));
                }

                {
                    uint32_t t = (uint32_t)si + (uint32_t)cx;
                    si = (uint16_t)t;
                    uint8_t c = (uint8_t)(t >> 16);
                    al = (uint8_t)(al + bl + c);

                    t = (uint32_t)di + (uint32_t)dx;
                    di = (uint16_t)t;
                    c = (uint8_t)(t >> 16);
                    ah = (uint8_t)(ah + bh + c);

                    moda_hi[i] = (uint16_t)((uint16_t)al | ((uint16_t)ah << 8));
                }

                {
                    uint32_t t = (uint32_t)si + (uint32_t)cx;
                    si = (uint16_t)t;
                    uint8_t c = (uint8_t)(t >> 16);
                    al = (uint8_t)(al + bl + c);

                    t = (uint32_t)di + (uint32_t)dx;
                    di = (uint16_t)t;
                    c = (uint8_t)(t >> 16);
                    ah = (uint8_t)(ah + bh + c);

                    modb_hi[i] = (uint16_t)((uint16_t)al | ((uint16_t)ah << 8));
                }
            }
        }

        Xadd = asm_style_scale307(Xadd);
        Yadd = asm_style_scale307(Yadd);

        uint8_t * dst = (uint8_t *)zoomer_planar_buffer;
        for (int row = 0; row < static_cast<int>(ZOOMYW); ++row)
        {
            // y pos step then base
            ypos += Yadd;
            xpos += Xadd;

            // base = ( (ypos>>8)<<8 | (xpos>>16)&0xFF ), 16-bit wrapped addressing
            uint16_t base = (uint16_t)((((uint32_t)ypos >> 8) & 0xFF00u) | (((uint32_t)xpos >> 16) & 0x00FFu));

            for (int i = 0; i < static_cast<int>(ZOOMXW / 4); ++i)
            {
                uint16_t offA0 = moda_lo[i], offA1 = moda_hi[i];
                uint16_t offB0 = modb_lo[i], offB1 = modb_hi[i];

                uint8_t loA = (uint8_t)src[(uint16_t)(base + offA0)];
                uint8_t hiA = (uint8_t)src[(uint16_t)(base + offA1)];
                *dst++ = loA;
                *dst++ = hiA;

                uint8_t loB = (uint8_t)src[(uint16_t)(base + offB0)];
                uint8_t hiB = (uint8_t)src[(uint16_t)(base + offB1)];
                *dst++ = loB;
                *dst++ = hiB;
            }
        }
    }

    void drawlens(int x0, int y0)
    {
        int32_t u1 = (x0 - lensxs) + (int32_t)(y0 - lensys) * 320L;
        int32_t u2 = (x0 - lensxs) + (int32_t)(y0 + lensys - 1) * 320L;
        int ys = lenshig / 2;
        int ye = lenshig - 1;

        for (int y = 0; y < ys; y++)
        {
            if (u1 >= 0 && u1 <= SCREEN_SIZE)
            {
                dorowC(lens1, (unsigned)u1, y, 0x40);  // lens distortion
                dorow2C(lens2, (unsigned)u1, y, 0x80); // lens graphics overlay
                dorow2C(lens3, (unsigned)u1, y, 0xC0); // lens graphics overlay
                dorow3C(lens4, (unsigned)u1, y);       // clear up distortion on previous frame
            }
            u1 += SCREEN_WIDTH;
            if (u2 >= 0 && u2 <= SCREEN_SIZE)
            {
                dorowC(lens1, (unsigned)u2, ye - y, 0x40);
                dorow2C(lens2, (unsigned)u2, ye - y, 0x80);
                dorow2C(lens3, (unsigned)u2, ye - y, 0xC0);
                dorow3C(lens4, (unsigned)u2, ye - y);
            }
            u2 -= SCREEN_WIDTH;
        }
    }

    void part1()
    {
        int x, y;
        int a, b, c;
        int frame = 0;
        char *cp, *dp;

        frame = 0;

        for (b = 0; b < 200; b++)
        {
            a = b;
            firfade1a[b] = (19 + a / 5 + 4) & (~7);
            firfade2a[b] = (-(19 + (199 - a) / 5 + 4)) & (~7);
            firfade1[b] = 170 * 64 + (100 - b) * 50;
            firfade2[b] = 170 * 64 + (100 - b) * 50;
        }

        if (!Shim::isDemoFirstPart())
        {
            if (Music::getPlusFlags() > -30)
                while (!demo_wantstoquit() && Music::getPlusFlags() < -6)
                {
                    AudioPlayer::Update(true);
                }
        }

        demo_vsync();

        Music::setFrame(0);

        while (!demo_wantstoquit() && frame < 300)
        {
            if (frame < 80)
            {
                a = frame * 2;
                for (c = 0; c < 6; c++)
                {
                    cp = (char *)Shim::vram;
                    dp = (char *)back;
                    for (y = 0; y < 200; y++)
                    {
                        x = firfade1[y] >> 6;
                        if (x < 0) x = 0;
                        if (x > SCREEN_WIDTH) x = SCREEN_WIDTH;
                        cp[x] = dp[x];
                        x = firfade2[y] >> 6;
                        if (x < 0) x = 0;
                        if (x > SCREEN_WIDTH) x = SCREEN_WIDTH;
                        cp[x] = dp[x];
                        firfade1[y] += firfade1a[y];
                        firfade2[y] += firfade2a[y];
                        cp += SCREEN_WIDTH;
                        dp += SCREEN_WIDTH;
                    }
                }
            }

            demo_blit();

            a = demo_vsync();

            frame += a;
        }
    }

    void part2()
    {
        int x, y;
        int a;
        int frame = 0, uframe = 0;
        x = 65 * 64;
        y = -50 * 64;
        uframe = frame = 0;

        while (!demo_wantstoquit() && uframe < 715)
        {
            if (uframe < 96)
            {
                a = (uframe - 32) / 2;
                if (a < 0) a = 0;
                Common::setpalarea(lens_fade2 + a * 3 * 192, 64, 192);
            }

            x = pathdata1[frame * 2 + 0];
            y = pathdata1[frame * 2 + 1];

            drawlens(x, y);

            demo_blit();

            a = demo_vsync();
            uframe += a;
            if (a > 3) a = 3;
            frame += a;
        }

        while (!demo_wantstoquit() && uframe < 720)
        {
            uframe += demo_vsync(true);
        }
    }

    void part3()
    {
        int x, y, xa, ya;
        int a;
        int frame = 0;
        rotpic90 = back;

        for (x = 0; x < 256; x++)
        {
            for (y = 0; y < 256; y++)
            {
                rotpic90[x + y * 256] = rotpic[y + (255 - x) * 256];
            }
        }

        demo_vsync();

        Common::setpalarea(lens_fade + 64 * 64 * 3, 0, 64);

        {
            frame = 0;

            while (!demo_wantstoquit() && frame < 2000)
            {
                if (Music::getPlusFlags() > -4) break;

                x = pathdata2[frame * 4 + 0];
                y = pathdata2[frame * 4 + 1];
                xa = pathdata2[frame * 4 + 2];
                ya = pathdata2[frame * 4 + 3];

                rotateC(x, y, ya, xa);

                {
                    unsigned char * src = (unsigned char *)zoomer_planar_buffer;
                    unsigned char * dst = Shim::vram;
                    for (int lineY = 0; lineY < 100; lineY++)
                    {
                        unsigned char * linesrc = src;
                        for (int index = 0; index < 40; index++)
                        {
                            *dst++ = linesrc[0];
                            *dst++ = linesrc[0];
                            *dst++ = linesrc[2];
                            *dst++ = linesrc[2];
                            *dst++ = linesrc[1];
                            *dst++ = linesrc[1];
                            *dst++ = linesrc[3];
                            *dst++ = linesrc[3];
                            linesrc += 4;
                        }

                        linesrc = src;

                        for (int index = 0; index < 40; index++)
                        {
                            *dst++ = linesrc[0];
                            *dst++ = linesrc[0];
                            *dst++ = linesrc[2];
                            *dst++ = linesrc[2];
                            *dst++ = linesrc[1];
                            *dst++ = linesrc[1];
                            *dst++ = linesrc[3];
                            *dst++ = linesrc[3];
                            linesrc += 4;
                        }
                        src += 160;
                    }
                }

                frame += demo_vsync();
                if (frame > 2000 - 128)
                {
                    a = frame - (2000 - 128);
                    a /= 2;
                    if (a > 63) a = 63;
                    Common::setpalarea(lens_fade + a * 64 * 3, 0, 64);
                }
                if (frame < 16)
                {
                    Common::setpalarea(lens_fade + (64 + frame) * 64 * 3, 0, 64);
                }
                demo_blit();
            }
        }

        for (a = 0; a < static_cast<int>(PaletteByteCount); a++)
            palette[a] = 63;

        Common::setpalarea(palette, 0, PaletteColorCount);
    }

    void main()
    {
        CLEAR(lensexb);

        pathdata1 = nullptr;
        pathdata2 = nullptr;
        back = nullptr;
        rotpic = nullptr;
        rotpic90 = nullptr;

        lenswid = {};
        lenshig = {};
        lensxs = {};
        lensys = {};
        lens1 = {};
        lens2 = {};
        lens3 = {};
        lens4 = {};

        CLEAR(palette);

        CLEAR(pathdata);
        CLEAR(lens_fade);
        CLEAR(lens_fade2);

        CLEAR(zoomer_planar_buffer);

        CLEAR(_rotpic);

        CLEAR(firfade1);
        CLEAR(firfade2);
        CLEAR(firfade1a);
        CLEAR(firfade2a);

        memcpy(lensexb, Data::lensexbBase, Data::lensexbBase_size);

        int x, y;
        int a, r, g, b, c, i;
        char * cp;
        Shim::clearScreen();

        rotpic = _rotpic;

        Shim::outp(0x3c8, 0);
        for (a = 0; a < static_cast<int>(PaletteByteCount); a++)
            Shim::outp(0x3c9, 0);

        a = *(short *)(Data::lensexp + 2);
        pathdata1 = (short *)(Data::lensexp + 4);
        pathdata2 = (short *)(Data::lensexp + 4 + 2 * a);

        memcpy(palette, lensexb + 16, PaletteByteCount);

        back = (uint8_t *)(lensexb + 16 + PaletteByteCount);

        lenswid = *(short *)(Data::lensex0 + 0);
        lenshig = *(short *)(Data::lensex0 + 2);
        cp = (char *)(Data::lensex0 + 4);
        lensxs = lenswid / 2;
        lensys = lenshig / 2;
        for (i = 1; i < 4; i++)
        {
            r = *cp++;
            g = *cp++;
            b = *cp++;
            for (a = 0; a < 64 * 3; a += 3)
            {
                c = r + palette[a + 0];
                if (c > 63) c = 63;
                palette[a + i * 64 * 3 + 0] = static_cast<char>(c);
                c = g + palette[a + 1];
                if (c > 63) c = 63;
                palette[a + i * 64 * 3 + 1] = static_cast<char>(c);
                c = b + palette[a + 2];
                if (c > 63) c = 63;
                palette[a + i * 64 * 3 + 2] = static_cast<char>(c);
            }
        }
        lens1 = (int *)Data::lensex1;
        lens2 = (int *)Data::lensex2;
        lens3 = (int *)Data::lensex3;
        lens4 = (int *)Data::lensex4;
        cp = lens_fade;
        for (x = 0; x < 64; x++)
        {
            for (y = 0; y < 64 * 3; y++)
            {
                a = (palette[y] * (63 - x) + x * 63) / 63;
                *cp++ = static_cast<char>(a);
            }
        }
        for (x = 0; x < 16; x++)
        {
            for (y = 0; y < 64 * 3; y++)
            {
                a = palette[y] + (15 - x) * 5;
                if (a > 63) a = 63;
                *cp++ = static_cast<char>(a);
            }
        }
        cp = lens_fade2;
        for (x = 0; x < 32; x++)
        {
            for (y = 64 * 3; y < static_cast<int>(PaletteColorCount * 3); y++)
            {
                a = y % (64 * 3);
                a = (palette[y] - palette[a] * (31 - x) / 31);
                *cp++ = static_cast<char>(a);
            }
        }

        for (x = 0; x < 256; x++)
        {
            for (y = 0; y < 256; y++)
            {
                a = y * 10 / 11 - 36 / 2;

                if (a < 0 || a > 199) a = 0;

                a = back[x + 32 + a * SCREEN_WIDTH];

                rotpic[x + y * 256] = static_cast<char>(a);
            }
        }

        demo_vsync();

        Common::setpalarea(palette, 0, PaletteColorCount);

        if (!demo_wantstoquit()) part1();

        while (!demo_wantstoquit() && Music::getPlusFlags() < -20)
        {
            AudioPlayer::Update(true);
        }

        demo_vsync();

        if (!demo_wantstoquit()) part2();

        if (!demo_wantstoquit()) part3();
    }
}
