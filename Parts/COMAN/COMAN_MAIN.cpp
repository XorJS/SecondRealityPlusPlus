#include "Parts/Common.h"

#include "COMAN_Data.h"

namespace Coman
{
    const size_t THELOOP_STRIDE = SCREEN_WIDTH / 2;

    namespace
    {
        char combg[16]{};
        char combguse[90 * 160]{};
        unsigned char vbuf[16384 * 4]{};

        int cameralevel = 0;

        short theloop_xsina1 = 0;
        short theloop_ysina1 = 0;
        short theloop_xsina2 = 0;
        short theloop_ysina2 = 0;
        short theloop_heigth = 0;

        Palette palette{};
    }

    static inline int16_t read_i16_le(const unsigned char * p)
    {
        return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    }

    static inline unsigned add32_cf(uint32_t * dst, uint32_t src)
    {
        uint32_t a = *dst, r = a + src;

        *dst = r;

        return r < a; // CF
    }

    static inline unsigned adc32_cf(uint32_t * dst, uint32_t src, unsigned cf_in)
    {
        uint64_t s = (uint64_t)(*dst) + (uint64_t)src + (uint64_t)cf_in;

        *dst = (uint32_t)s;

        return (unsigned)(s >> 32);
    }

    static inline unsigned adc16_into_low_u32(uint32_t * reg32, int16_t imm, unsigned cf_in)
    {
        uint16_t lo = (uint16_t)(*reg32 & 0xFFFFu);
        uint32_t s = (uint32_t)lo + (uint32_t)(uint16_t)imm + (uint32_t)cf_in;
        *reg32 = (*reg32 & 0xFFFF0000u) | (s & 0xFFFFu);

        return (s > 0xFFFFu);
    }

    void theloop3(int xw, int yw, int b)
    {
        uint32_t EAX = 0;
        uint32_t ECX = 0xEC00FFFAu;
        uint32_t ESI = ((uint32_t)xw) & ~1u;
        uint32_t EDI = ((uint32_t)yw) & ~1u;
        int BP = b;

        for (size_t i = 0; i < Data::kBlocks_Size; ++i)
        {
            const Data::TheLoopBlock & blk = Data::kBlocks[i];

            if (blk.mode == 1)
            {
                uint16_t si = (uint16_t)ESI, di = (uint16_t)EDI;
                si = (uint16_t)(si + (uint16_t)theloop_xsina1);
                di = (uint16_t)(di + (uint16_t)theloop_ysina1);
                ESI = (ESI & 0xFFFF0000u) | si;
                EDI = (EDI & 0xFFFF0000u) | di;
            }
            else
            {
                uint16_t si = (uint16_t)ESI, di = (uint16_t)EDI;
                si = (uint16_t)(si + (uint16_t)theloop_xsina2);
                di = (uint16_t)(di + (uint16_t)theloop_ysina2);
                ESI = (ESI & 0xFFFF0000u) | si;
                EDI = (EDI & 0xFFFF0000u) | di;
            }

            int16_t BX = read_i16_le(Data::w1dta + ESI);
            BX = (int16_t)(BX + read_i16_le(Data::w2dta + EDI));
            BX = (int16_t)(BX + blk.bx_add);

            if ((int16_t)(uint16_t)(EAX & 0xFFFFu) >= BX)
            {
                if (blk.mode == 1)
                {
                    unsigned cf = add32_cf(&EAX, ECX);
                    (void)adc16_into_low_u32(&EAX, (int16_t)-1, cf);
                }
                else
                {
                    unsigned cf = add32_cf(&EAX, ECX);
                    cf = adc32_cf(&EAX, ECX, cf);
                    (void)adc16_into_low_u32(&EAX, (int16_t)-1, cf);
                }

                continue;
            }

            uint8_t DL = (uint8_t)((uint16_t)((uint16_t)BX + (uint16_t)blk.dx_add) & 0xFFu);
            DL >>= 1;

            for (;;)
            {
                unsigned cf = add32_cf(&EAX, blk.eax_inc);
                (void)adc16_into_low_u32(&EAX, 0, cf);

                vbuf[BP] = DL;

                uint16_t AXu = (uint16_t)(EAX & 0xFFFFu);
                if ((int16_t)AXu >= BX)
                {
                    unsigned cf1 = adc32_cf(&ECX, 0x00A00000u << 4, 0);
                    (void)adc16_into_low_u32(&ECX, 0, cf1);
                    BP -= THELOOP_STRIDE;

                    break;
                }

                cf = add32_cf(&EAX, blk.eax_inc);

                (void)adc16_into_low_u32(&EAX, 0, cf);

                vbuf[BP - THELOOP_STRIDE] = DL;

                AXu = (uint16_t)(EAX & 0xFFFFu);
                if ((int16_t)AXu >= BX)
                {
                    BP -= THELOOP_STRIDE;

                    unsigned cf_add = add32_cf(&ECX, 0x00A00000u << 4);
                    unsigned cf2 = adc32_cf(&ECX, 0x00A00000u << 4, cf_add);
                    (void)adc16_into_low_u32(&ECX, 0, cf2);
                    BP -= THELOOP_STRIDE;

                    break;
                }

                cf = add32_cf(&EAX, blk.eax_inc);
                (void)adc16_into_low_u32(&EAX, 0, cf);

                vbuf[BP - THELOOP_STRIDE * 2] = DL;

                unsigned cf_add = add32_cf(&ECX, 0x01E00000u << 4);
                (void)adc16_into_low_u32(&ECX, 0, cf_add);

                BP -= THELOOP_STRIDE * 3;

                AXu = (uint16_t)(EAX & 0xFFFFu);
                if ((int16_t)AXu >= BX) break;
            }

            if (blk.mode == 1)
            {
                unsigned cf = add32_cf(&EAX, ECX);
                (void)adc16_into_low_u32(&EAX, (int16_t)-1, cf);
            }
            else
            {
                unsigned cf = add32_cf(&EAX, ECX);
                cf = adc32_cf(&EAX, ECX, cf);
                (void)adc16_into_low_u32(&EAX, (int16_t)-1, cf);
            }
        }
    }

    void new_docol(int xw, int yw, int xa, int ya, int b)
    {
        theloop_heigth = static_cast<short>(cameralevel);

        theloop_xsina1 = (xa & ~1);
        theloop_ysina1 = (ya & ~1);

        theloop_xsina2 = theloop_xsina1 << 1;
        theloop_ysina2 = theloop_ysina1 << 1;

        theloop3(xw & 0xFFFF, yw & 0xFFFF, b);
    }

    void docopy(void * dest, int startrise)
    {
        uint8_t * dst = (uint8_t *)dest;

        const uint8_t * src0 = vbuf + 60 * 160;

        uint8_t * clear_base = dst + 52 * 80 * 4;
        memset(clear_base, 0, 18 * 80 * 4);

        int lines = 140 - startrise;
        if (lines <= 0) return;

        uint8_t * dst_line = clear_base + 18 * 80 * 4 + (size_t)startrise * SCREEN_WIDTH;

        for (int y = 0; y < lines; ++y)
        {
            const uint8_t * s = src0 + y * 160;
            uint8_t * d = dst_line + y * SCREEN_WIDTH;

            for (int z = 0; z < 80; ++z)
            {
                uint8_t a = s[z];
                d[4 * z + 0] = a;
                d[4 * z + 1] = a;
            }

            for (int z = 0; z < 80; ++z)
            {
                uint8_t b = s[z + 80];
                d[4 * z + 2] = b;
                d[4 * z + 3] = b;
            }
        }

        memset(vbuf + 68 * 160, 0, 30 * 160);
    }

    void coman_doit()
    {
        int startrise = 160, frepeat = 1;
        int a, b, x, y, xa, ya, xw, yw, r;
        int rot2 = 0, rot = 0, rsin, rcos, xwav = 0, ywav = 0;
        int rsin2, rcos2;
        int frame = 0;
        rot = 0;
        rot2 = 0;
        cameralevel = -270;

        demo_vsync();

        memset(vbuf, 0, sizeof(vbuf));

        if (!Shim::isDemoFirstPart())
        {
            while (!demo_wantstoquit() && Music::getPlusFlags() < 0)
            {
                AudioPlayer::Update(true);
            }
        }

        while (!demo_wantstoquit() && frame < 4444)
        {
            a = Music::getPlusFlags();

            if (a > -8 && a < 0) break;

            while (frepeat--)
            {
                if (a > -30 && a < 0)
                {
                    if (startrise < 160) startrise += 1;
                }
                if (frame < 400 && startrise > 0)
                {
                    if (frame == 4)
                    {
                        demo_vsync();

                        Common::setpalarea(palette, 0, PaletteColorCount);
                    }

                    if (startrise > 0) startrise -= 1;
                }
            }

            rot2 += 4;
            rot += (Common::Data::sin1024[rot2 & 1023]) / 15;
            r = rot >> 3;
            rsin = Common::Data::sin1024[r & 1023];
            rcos = Common::Data::sin1024[(r + 256) & 1023];
            rsin2 = Common::Data::sin1024[(r + 177) & 1023];
            rcos2 = Common::Data::sin1024[(r + 177 + 256) & 1023];
            xw = xwav;
            yw = ywav;

            for (a = 0; a < 160; a++)
            {
                x = (a - 80);
                y = 160;
                xa = (int)(((int32_t)x * (int32_t)rcos + (int32_t)y * (int32_t)rsin) / 256L);
                ya = (int)(((int32_t)y * (int32_t)rcos2 - (int32_t)x * (int32_t)rsin2) / 256L);
                b = (a & 1) * 80 + (a >> 1) + 199 * 160;

                new_docol(xw & 0xFFFF, yw & 0xFFFF, xa, ya, b);

                if (a == 80)
                {
                    xwav += xa * 4;
                    ywav += ya * 4;
                }
            }

            frame += (frepeat = demo_vsync());

            if (startrise < 140)
            {
                docopy(Shim::vram, startrise);
            }

            demo_blit();
        }
    }

    void main()
    {
        CLEAR(combg);
        CLEAR(combguse);
        CLEAR(vbuf);

        cameralevel = 0;

        theloop_xsina1 = 0;
        theloop_ysina1 = 0;
        theloop_xsina2 = 0;
        theloop_ysina2 = 0;
        theloop_heigth = 0;

        CLEAR(palette);

        unsigned int uc;
        int a, b, x, y;

        Shim::clearScreen();

        for (a = 0; a < PaletteColorCount; a++)
        {
            uc = (223 - a * 22 / 26) * 3;
            b = (230 - a) / 4;
            b += Common::Data::sin1024[a * 4 & 1023] / 32;

            if (b < 0) b = 0;
            if (b > 63) b = 63;

            palette[uc + 1] = static_cast<char>(b);
            b = (255 - a) / 3;

            if (b > 63) b = 63;

            palette[uc + 2] = static_cast<char>(b);

            b = a - 220;

            if (b < 0) b = -b;
            if (b > 40) b = 40;

            b = 40 - b;
            palette[uc + 0] = static_cast<char>(b / 3);
        }

        for (a = 0; a < PaletteByteCount - 16 * 3; a++)
        {
            b = palette[a];
            b = b * 9 / 6;

            if (b > 63) b = 63;

            palette[a] = static_cast<char>(b);
        }

        for (a = 0; a < 24; a++)
        {
            uc = (255 - a) * 3;
            b = a - 4;
            if (b < 0) b = 0;
            palette[uc + 0] = static_cast<char>(b / 2);
            palette[uc + 1] = 0;
            palette[uc + 2] = 0;
        }

        palette[0] = 0;
        palette[1] = 0;
        palette[2] = 0;

        for (x = (PaletteColorCount - 16) * 3; x < PaletteByteCount; x++)
        {
            palette[x] = combg[16 + x];
        }

        for (y = 0; y < 90; y++)
        {
            for (x = 0; x < 80; x++)
            {
                combguse[x + y * 160] = combg[x * 4 + y * SCREEN_WIDTH + PaletteByteCount + 16];
            }
            for (x = 0; x < 80; x++)
            {
                combguse[x + 80 + y * 160] = combg[x * 4 + y * SCREEN_WIDTH + 2 + PaletteByteCount + 16];
            }
        }

        demo_vsync();

        Common::setpalarea(palette, 0, PaletteColorCount);

        memset(Shim::vram, 0, SCREEN_SIZE);

        demo_vsync();

        Common::setpalarea(palette, 0, PaletteColorCount);

        coman_doit();
    }
}
