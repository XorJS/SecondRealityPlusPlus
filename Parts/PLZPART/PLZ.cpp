#include <math.h>

#include "Parts/Common.h"

#include "PLZ_Data.h"

#define SX sinit[kx]
#define SY sinit[ky]
#define SZ sinit[kz]
#define CX kosinit[kx]
#define CY kosinit[ky]
#define CZ kosinit[kz]

namespace PLZ
{
    constexpr const float F_PI = 3.141592f;

    constexpr const size_t MAXY = 280;
    constexpr const size_t YADD = 0;

    constexpr const size_t Size = 84;

    struct polygons_to_draw
    {
        int p;
        int dis;
    };

    struct t_plz_object
    {
        char name[100]{};

        int pnts{};
        struct points_3d
        {
            int x{};
            int y{};
            int z{};
            int xx{};
            int yy{};
            int zz{};
            int xxx{};
            int yyy{};
        } point[256]{};

        int faces{};
        struct polygon
        {
            int p1{};
            int p2{};
            int p3{};
            int p4{};
            int p5{};
            int p6{};
            int n{};
            int color{};
        } pg[256]{};

        int lines{};
        struct lines
        {
            int p1{};
            int p2{};
            int n{};
            int col{};
        } lin[256]{};
    };

    namespace
    {
        const int timetable[10] = { 64 * 6 * 2 - 45, 64 * 6 * 4 - 45, 64 * 6 * 5 - 45, 64 * 6 * 6 - 45, 64 * 6 * 7 + 90, 0 };

        unsigned char * to = nullptr;   /* VRAM scan pointer (vmem + yy*640) */
        unsigned char * from = nullptr; /* texture base (64K) */
        short * ctau = nullptr;         /* [minX, maxX] per scanline */
        const uint8_t * dseg = nullptr; /* row-table base (byte address) */

        short yy = 0;
        int32_t ax1 = 0, ax2 = 0, xx1 = 0, xx2 = 0;
        int32_t txx1 = 0, txy1 = 0, tax1 = 0, tay1 = 0;
        int32_t txx2 = 0, txy2 = 0, tax2 = 0, tay2 = 0;

        int sini[2000]{};
        char vram_half[640 * 134]{};
        char (*vmem)[640] = reinterpret_cast<char (*)[640]>(vram_half);

        char kuva1[64][256]{};
        char kuva2[64][256]{};
        char kuva3[64][256]{};
        char dist1[128][256]{};

        char sinx[128]{}, siny[128]{};

        char plz_pal[PaletteByteCount]{};

        short clrtau[8][256][2]{};
        int clrptr = 0;

        char plz_vidmem[2][Size * 280]{};
        unsigned short pals[6][PaletteByteCount]{};

        int curpal = 0;
        int ttptr = 0;

        int l1 = 1000, l2 = 2000, l3 = 3000, l4 = 4000;
        int k1 = 3500, k2 = 2300, k3 = 3900, k4 = 3670;

        int il1 = 1000, il2 = 2000, il3 = 3000, il4 = 4000;
        int ik1 = 3500, ik2 = 2300, ik3 = 3900, ik4 = 3670;

        char pompi = 0;

        char plz_fadepal[2 * PaletteByteCount] = { 0 };

        int drop_y = 0;

        const uint8_t * LC1[Size]{}; // psini   + c1 + 8*ccc
        const uint8_t * LC2[Size]{}; // lsini16 + (c2<<1) + (80*8 - 8*ccc)
        const uint8_t * LC3[Size]{}; // psini   + c3 + (80*4 - 4*ccc)
        const uint8_t * LC4[Size]{}; // lsini4  + (c4<<1) + 32*ccc

        const short * sinit = (const short *)Data::_sinit;
        const short * kosinit = (const short *)Data::_kosinit;

        char * kuvataus[] = { (char *)kuva1, (char *)kuva2, (char *)kuva3, (char *)kuva1 };
        char * disttaus[] = { (char *)dist1, (char *)dist1, (char *)dist1, (char *)dist1 };

        char fpal[PaletteByteCount]{};

        int plz_polys = 0;
        int light_src[6] = { 0 };
        int lls[6] = { 0 };

        int cxx{}, cxy{}, cxz{}, cyx{}, cyy{}, cyz{}, czx{}, czy{}, czz{};
        short kx = 0, ky = 0, kz = 0, dis = 320, ltx = 0, ty = -50;
        short ls_kx = 0, ls_ky = 0, ls_kz = 0, ls_x = 0, ls_y = 0, ls_z = 128;
        int page = 0;
        int frames = 0;

        polygons_to_draw ptodraw[256]{};

        t_plz_object plz_object = { "Cube",
                                    8, // points
                                    {
                                        { 125, 125, 125 },
                                        { 125, -125, 125 },
                                        { -125, -125, 125 },
                                        { -125, 125, 125 },
                                        { 125, 125, -125 },
                                        { 125, -125, -125 },
                                        { -125, -125, -125 },
                                        { -125, 125, -125 },
                                    },
                                    6, // faces
                                    { { 1, 2, 3, 0, 0, 0, 0, 0 },
                                      { 7, 6, 5, 4, 0, 0, 0, 0 },
                                      { 0, 4, 5, 1, 0, 0, 0, 1 },
                                      { 1, 5, 6, 2, 0, 0, 0, 2 },
                                      { 2, 6, 7, 3, 0, 0, 0, 1 },
                                      { 3, 7, 4, 0, 0, 0, 0, 2 } } };
    }

    void swappage()
    {
        page = (page + 1) % 6;

        if (page == 0)
        {
            Common::cop_start = 0xaa00 + 40;
            Common::cop_scrl = 4;
        }
        else if (page == 1)
        {
            Common::cop_start = 0x0000 + 40;
            Common::cop_scrl = 0;
        }
        else if (page == 2)
        {
            Common::cop_start = 0x5500 + 40;
            Common::cop_scrl = 4;
        }
        else if (page == 3)
        {
            Common::cop_start = 0xaa00 + 40;
            Common::cop_scrl = 0;
        }
        else if (page == 4)
        {
            Common::cop_start = 0x0000 + 40;
            Common::cop_scrl = 4;
        }
        else if (page == 5)
        {
            Common::cop_start = 0x5500 + 40;
            Common::cop_scrl = 0;
        }
    }

    void initpparas()
    {
        l1 = il1;
        l2 = il2;
        l3 = il3;
        l4 = il4;

        k1 = ik1;
        k2 = ik2;
        k3 = ik3;
        k4 = ik4;
    }

    void moveplz()
    {
        // k* updates
        k1 = (uint16_t)((k1 - 3) & 0x0FFF);
        k2 = (uint16_t)((k2 - 2) & 0x0FFF);
        k3 = (uint16_t)((k3 + 1) & 0x0FFF);
        k4 = (uint16_t)((k4 + 2) & 0x0FFF);

        // l* updates
        l1 = (uint16_t)((l1 - 1) & 0x0FFF);
        l2 = (uint16_t)((l2 - 2) & 0x0FFF);
        l3 = (uint16_t)((l3 + 2) & 0x0FFF);
        l4 = (uint16_t)((l4 + 3) & 0x0FFF);
    }

    void do_drop()
    {
        if (Common::cop_drop = (uint16_t)(Common::cop_drop + 1); Common::cop_drop <= (uint16_t)64u)
        {
            const uint8_t * dtau_bytes = (const uint8_t *)Data::plz_dtau;
            uint16_t off = (uint16_t)(Common::cop_drop << 1);
            uint16_t v = (uint16_t)((uint16_t)dtau_bytes[off] | (uint16_t)(dtau_bytes[off + 1] << 8));
            drop_y = (uint32_t)v;

            return;
        }

        if (Common::cop_drop >= (uint16_t)256u)
        {
            Common::cop_drop = 0;
            return;
        }

        if (Common::cop_drop < (uint16_t)128u)
        {
            if (Common::cop_drop > (uint16_t)(64u + 32u))
            {
                Common::cop_drop = 0;

                return;
            }
        }

        Common::cop_pal = plz_fadepal;
        Common::do_pal = 1;

        if (Common::cop_drop == (uint16_t)65u)
        {
            drop_y = (uint32_t)Data::plz_dtau[0];

            initpparas();

            return;
        }

        drop_y = (uint32_t)Data::plz_dtau[0];

        // Palette accumulation
        const uint8_t * src = (const uint8_t *)Common::cop_fadepal;
        uint8_t * acc_hi = (uint8_t *)plz_fadepal;
        uint8_t * acc_lo = (uint8_t *)(plz_fadepal + 768);

        for (int i = 0; i < 768; ++i)
        {
            uint16_t ax = (uint16_t)src[(i << 1)] | ((uint16_t)src[(i << 1) + 1] << 8);
            uint8_t al = (uint8_t)(ax & 0xFF);
            uint8_t ah = (uint8_t)(ax >> 8);

            // add low byte
            uint16_t lo = (uint16_t)acc_lo[i] + (uint16_t)al;
            acc_lo[i] = (uint8_t)(lo & 0xFF);

            // adc high byte with carry from low
            uint16_t hi = (uint16_t)acc_hi[i] + (uint16_t)ah + (uint16_t)(lo >> 8);
            acc_hi[i] = (uint8_t)(hi & 0xFF);
        }
    }

    static inline uint16_t rd16le(const uint8_t * p, const uint8_t * minP)
    {
        p = minP + (((p - minP) & (Data::sinBufferSize - 1)) & (~1));

        return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    }

    static inline uint8_t rd8le(const uint8_t * p, const uint8_t * minP)
    {
        p = minP + ((p - minP) & (Data::sinBufferSize - 1));

        return p[0];
    }

    // Runtime ccc order inside the unrolled loop: 3,2,1,0, 7,6,5,4, ...
    static inline int ccc_from_k(int k)
    {
        return (k & ~3) + (3 - (k & 3));
    }

    int setplzparas(int c1, int c2, int c3, int c4)
    {
        const uint32_t C1 = (uint16_t)c1;
        const uint32_t C2 = (uint16_t)c2;
        const uint32_t C3 = (uint16_t)c3;
        const uint32_t C4 = (uint16_t)c4;

        for (int ccc = 0; ccc < static_cast<int>(Size); ++ccc)
        {
            LC1[ccc] = Data::psini + (C1 + 8 * ccc);
            LC2[ccc] = ((const uint8_t *)Data::lsini16) + ((C2 << 1) + (80 * 8 - 8 * ccc));
            LC3[ccc] = Data::psini + (C3 + 80 * 4 - 4 * ccc);
            LC4[ccc] = ((const uint8_t *)Data::lsini4) + ((C4 << 1) + 32 * ccc);
        }

        return 0;
    }

    int plzline(int y, char * vseg)
    {
        const uint32_t y2 = ((uint32_t)(uint16_t)y) << 1;

        uint8_t out[Size];

        for (int k = 0; k < static_cast<int>(COUNT(out)); ++k)
        {
            const int ccc = ccc_from_k(k);
            const uint16_t bx1 = rd16le(LC2[ccc] + y2, (const uint8_t *)Data::lsini16);
            const uint8_t s1 = rd8le(LC1[ccc] + (uint32_t)bx1, Data::psini);
            const uint16_t bx2 = rd16le(LC4[ccc] + y2, (const uint8_t *)Data::lsini4);
            const uint8_t s2 = rd8le(LC3[ccc] + (uint32_t)bx2 + y2, Data::psini);

            out[ccc] = (uint8_t)(s1 + s2);
        }

        memcpy(vseg, out, COUNT(out));

        return 0;
    }

    void init_plz()
    {
        int a;
        unsigned short * pptr = (unsigned short *)pals;

        Common::cop_start = 96 * (682 - 400);

        for (a = 0; a < static_cast<int>(PaletteColorCount); a++)
            Shim::setpal(a, 63, 63, 63);

        // RGB
        pptr = &pals[0][3];
        for (a = 1; a < 64; a++)
            *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[63 - a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[a];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[63 - a];

        // RB-black
        pptr = &pals[1][3];
        for (a = 1; a < 64; a++)
            *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[63 - a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[a];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[63 - a];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[63], *pptr++ = Data::ptau[a];

        // RB-white
        pptr = &pals[3][3];
        for (a = 1; a < 64; a++)
            *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[63], *pptr++ = Data::ptau[a], *pptr++ = Data::ptau[a];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[63 - a], *pptr++ = Data::ptau[63 - a], *pptr++ = Data::ptau[63];
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[0], *pptr++ = Data::ptau[63];

        // white
        pptr = &pals[2][3];
        for (a = 1; a < 64; a++)
            *pptr++ = Data::ptau[0] / 2, *pptr++ = Data::ptau[0] / 2, *pptr++ = Data::ptau[0] / 2;
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[a] / 2, *pptr++ = Data::ptau[a] / 2, *pptr++ = Data::ptau[a] / 2;
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[63 - a] / 2, *pptr++ = Data::ptau[63 - a] / 2, *pptr++ = Data::ptau[63 - a] / 2;
        for (a = 0; a < 64; a++)
            *pptr++ = Data::ptau[0] / 2, *pptr++ = Data::ptau[0] / 2, *pptr++ = Data::ptau[0] / 2;

        // white II
        pptr = &pals[4][3];
        for (a = 1; a < 75; a++)
            *pptr++ = Data::ptau[63 - a * 64 / 75], *pptr++ = Data::ptau[63 - a * 64 / 75], *pptr++ = Data::ptau[63 - a * 64 / 75];
        for (a = 0; a < 106; a++)
            *pptr++ = 0, *pptr++ = 0, *pptr++ = 0;
        for (a = 0; a < 75; a++)
            *pptr++ = Data::ptau[a * 64 / 75] * 8 / 10, *pptr++ = Data::ptau[a * 64 / 75] * 9 / 10, *pptr++ = Data::ptau[a * 64 / 75];

        pptr = (unsigned short *)pals;
        for (a = 0; a < static_cast<int>(PaletteByteCount); a++, pptr++)
            *pptr = (*pptr - 63) * 2;
        for (a = PaletteByteCount; a < static_cast<int>(PaletteByteCount * 5); a++, pptr++)
            *pptr *= 8;
    }

    void plz()
    {
        int y{};

        demo_changemode(SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, 0.0f);

        if (!Shim::isDemoFirstPart())
        {
            while (Music::getPlusFlags() < 0 && !demo_wantstoquit())
            {
                AudioPlayer::Update(true);
            }
        }

        Music::setFrame(0);

        init_plz();
        Common::cop_drop = 128;
        memset(Common::fadepal, 0, PaletteByteCount);
        memset(Common::fadepal_short, 0, PaletteByteCount * 2);
        Common::cop_fadepal = (short *)(pals[curpal++]);
        drop_y = Data::plz_dtau[0];

        Common::frame_count = 0;

        initpparas();

        while (!demo_wantstoquit())
        {
            Common::frame_count = 0;

            if (Music::getFrame() > timetable[ttptr])
            {
                memset(plz_fadepal, 0, PaletteByteCount);
                Common::cop_drop = 1;
                Common::cop_fadepal = (short *)(pals[curpal++]);
                ttptr++;

                il1 = Data::inittable[ttptr][0];
                il2 = Data::inittable[ttptr][1];
                il3 = Data::inittable[ttptr][2];
                il4 = Data::inittable[ttptr][3];
                ik1 = Data::inittable[ttptr][4];
                ik2 = Data::inittable[ttptr][5];
                ik3 = Data::inittable[ttptr][6];
                ik4 = Data::inittable[ttptr][7];
            }

            if (curpal == 4 && Common::cop_drop > 64) break;

            setplzparas(k1, k2, k3, k4);
            for (y = 0; y < static_cast<int>(MAXY); y += 2)
                plzline(y, plz_vidmem[0] + y * Size + YADD * 6);

            setplzparas(l1, l2, l3, l4);
            for (y = 1; y < static_cast<int>(MAXY); y += 2)
                plzline(y, plz_vidmem[0] + y * Size + YADD * 6);

            setplzparas(k1, k2, k3, k4);
            for (y = 1; y < static_cast<int>(MAXY); y += 2)
                plzline(y, plz_vidmem[1] + y * Size + YADD * 6);

            setplzparas(l1, l2, l3, l4);
            for (y = 0; y < static_cast<int>(MAXY); y += 2)
                plzline(y, plz_vidmem[1] + y * Size + YADD * 6);

            if (Common::cop_drop)
            {
                do_drop();
            }

            Common::copper2();

            moveplz();

            {
                char * src1 = plz_vidmem[0];
                char * src2 = plz_vidmem[1];
                char * dst = (char *)(Shim::vram + drop_y * SCREEN_WIDTH);

                memset(Shim::vram, 0, SCREEN_WIDTH * DOUBlE_SCREEN_HEIGHT);

                for (int index = 0; index < static_cast<int>(MAXY); index++)
                {
                    if (index + drop_y >= DOUBlE_SCREEN_HEIGHT)
                    {
                        break;
                    }
                    src1 += 2;
                    src2 += 2;
                    for (int x = 0; x < 80; x++)
                    {
                        *dst++ = *src2;
                        *dst++ = *src1;
                        *dst++ = *src2++;
                        *dst++ = *src1++;
                    }
                    src1 += 2;
                    src2 += 2;
                }
            }

            demo_blit();

            Common::frame_count += demo_vsync();
        }
    }

    void shadepal(uint8_t * destPal, const uint8_t * ppal, int shade)
    {
        const uint8_t dl = (uint8_t)shade;

        for (int i = 0; i < 192; ++i)
        {
            uint16_t ax = (uint16_t)ppal[i] * (uint16_t)dl;
            ax >>= 6;
            destPal[i] = (uint8_t)ax;
        }
    }

    int getspl(int where)
    {
        unsigned idx4 = (unsigned)((where >> 8) & 0xFFFF);
        unsigned t = (unsigned)(where & 0xFF); /* 0..255 sub-step */
        unsigned wofs = t * 1u;                /* word index within each band */

        /* bands are 256 words apart (64*8 bytes) */
        const short * w0 = &Data::splinecoef[wofs + 0];
        const short * w1 = &Data::splinecoef[wofs + 256];
        const short * w2 = &Data::splinecoef[wofs + 512];
        const short * w3 = &Data::splinecoef[wofs + 768];

        /* sum across 4 successive keyframes: (i+3,i+2,i+1,i+0) for each of 8 channels */
        short out[8]{};

        for (int comp = 0; comp < 8; ++comp)
        {
            /* grab value for this channel from each of the 4 keyframes */
            int32_t a3 = Data::buu[idx4 + 3][comp];
            int32_t a2 = Data::buu[idx4 + 2][comp];
            int32_t a1 = Data::buu[idx4 + 1][comp];
            int32_t a0 = Data::buu[idx4 + 0][comp];

            int32_t s = 0;
            s += a3 * (int32_t)*w0; /* [splinecoef + edi]           */
            s += a2 * (int32_t)*w1; /* [splinecoef + edi + 64*8]    */
            s += a1 * (int32_t)*w2; /* [splinecoef + edi + 128*8]   */
            s += a0 * (int32_t)*w3; /* [splinecoef + edi + 192*8]   */

            out[comp] = (short)((s + (s >= 0 ? 0 : ((1L << 15) - 1))) >> 15); /* arithmetic >> 15 */
        }

        ltx = out[0];
        ty = out[1];
        dis = out[2];
        kx = out[3];
        ky = out[4];
        kz = out[5];
        ls_kx = out[6];
        ls_ky = out[7];

        return 0;
    }

    void count_const()
    {
        // matrix equations:
        //X Y Z -> nX
        //X Y Z -> nY
        //X Y Z -> nZ
        //
        // 0=Ycos*Zcos         2=Ycos*Zsin         4=-Ysin
        // 6=Xsin*Zcos*Ysin     8=Xsin*Ysin*Zsin    10=Ycos*Xsin
        //   -Xcos*Zsin           +Xcos*Zcos
        //12=Xcos*Zcos*Ysin    14=Xcos*Ysin*Zsin    16=Ycos*Xcos
        //   +Xsin*Zsin           -Xsin*Zcos

        cxx = (int32_t)CY * (int32_t)CZ >> (15 + 7);
        cxy = (int32_t)CY * (int32_t)SZ >> (15 + 7);
        cxz = -(int32_t)SY >> 7;

        cyx = ((((int32_t)SX * (int32_t)CZ + 16384L) >> 15) * (int32_t)SY - ((int32_t)CX * (int32_t)SZ)) >> (15 + 7);
        cyy = ((((int32_t)SX * (int32_t)SY + 16384L) >> 15) * (int32_t)SZ + ((int32_t)CX * (int32_t)CZ)) >> (15 + 7);
        cyz = (int32_t)CY * (int32_t)SX >> (15 + 7);

        czx = ((((int32_t)CX * (int32_t)CZ + 16384L) >> 15) * (int32_t)SY + (int32_t)SX * (int32_t)SZ) >> (15 + 7);
        czy = ((((int32_t)CX * (int32_t)SY + 16384L) >> 15) * (int32_t)SZ - (int32_t)SX * (int32_t)CZ) >> (15 + 7);
        czz = (int32_t)CY * (int32_t)CX >> (15 + 7);
    }

    void plz_rotate()
    {
        int a, x, y, z, xx, lyy, zz;

        for (a = 0; a < plz_object.pnts; a++)
        {
            x = plz_object.point[a].x;
            y = plz_object.point[a].y;
            z = plz_object.point[a].z;

            plz_object.point[a].xx = xx = (((x * cxx >> 1) + (y * cxy >> 1) + (z * cxz >> 1)) >> 7) + ltx;
            plz_object.point[a].yy = lyy = (((x * cyx >> 1) + (y * cyy >> 1) + (z * cyz >> 1)) >> 7) + ty;
            plz_object.point[a].zz = zz = (((x * czx >> 1) + (y * czy >> 1) + (z * czz >> 1)) >> 7) + dis;

            plz_object.point[a].xxx = (xx * 256L) / zz + 160 + 160;
            plz_object.point[a].yyy = (lyy * 142L) / zz + 66;
        }
    }

    void sort_faces()
    {
        int a = 0, c, x, y, z, p = 0;
        int32_t ax, ay, az, bx, by, bz, lkx, lky, lkz, nx, ny, nz, s;

        while (a < plz_object.faces)
        {
            x = plz_object.point[plz_object.pg[a].p1].xx;
            y = plz_object.point[plz_object.pg[a].p1].yy;
            z = plz_object.point[plz_object.pg[a].p1].zz;

            ax = plz_object.point[plz_object.pg[a].p2].xx - x;
            ay = plz_object.point[plz_object.pg[a].p2].yy - y;
            az = plz_object.point[plz_object.pg[a].p2].zz - z;

            bx = plz_object.point[plz_object.pg[a].p3].xx - x;
            by = plz_object.point[plz_object.pg[a].p3].yy - y;
            bz = plz_object.point[plz_object.pg[a].p3].zz - z;

            nx = ay * bz - az * by;
            ny = az * bx - ax * bz;
            nz = ax * by - ay * bx; // normal

            lkx = -x;
            lky = -y;
            lkz = -z; // view_vector

            s = lkx * nx + lky * ny + lkz * nz; // skalaaritulo

            if (s > 0)
            {
                a++;
                continue;
            }

            s = (ls_x * nx + ls_y * ny + ls_z * nz) / 250000 + 32;
            light_src[p] = s;
            c = plz_object.pg[a].color;
            if (lls[p] != light_src[p])
            {
                shadepal((uint8_t *)&fpal[c * 64 * 3], (uint8_t *)&plz_pal[c * 64 * 3], light_src[p]);

                lls[p] = light_src[p];
            }

            ptodraw[p++].p = a++;
        }
        plz_polys = p;
    }

    static inline uint8_t start_mask4(unsigned m)
    {
        static const uint8_t v[4] = { 0x0F, 0x0E, 0x0C, 0x08 };
        return v[m & 3];
    }

    static inline uint8_t end_mask4(unsigned m)
    {
        static const uint8_t v[4] = { 0x01, 0x03, 0x07, 0x0F };
        return v[m & 3];
    }

    static inline void poke4_full(uint8_t * d, uint8_t c)
    {
        d[0] = c;
        d[1] = c;
        d[2] = c;
        d[3] = c;
    }

    static inline void poke4_mask(uint8_t * d, uint8_t c, uint8_t m)
    {
        if (m & 1) d[0] = c;
        if (m & 2) d[1] = c;
        if (m & 4) d[2] = c;
        if (m & 8) d[3] = c;
    }

    static inline uint16_t bx_from_txy(uint32_t tX, uint32_t tY)
    {
        return (uint16_t)((((tY >> 16) & 0xFFu) << 8) | ((tX >> 16) & 0xFFu));
    }

    static inline uint8_t sample_bx(uint16_t bx)
    {
        const uint8_t * rt = dseg;

        return ((const uint8_t *)from)[bx + rt[bx]];
    }

    void do_block(short ycount)
    {
        if (ycount <= 0) return;

        while (ycount--)
        {
            if (yy >= 134) break;

            if (yy >= 0)
            {
                const int16_t xL = (int16_t)(xx2 >> 16);
                const int16_t xR = (int16_t)(xx1 >> 16);

                // Record span for clear(): min(left), max(right)
                if (xL <= ctau[0]) ctau[0] = xL;
                if (xR >= ctau[1]) ctau[1] = xR;

                // 4-pixel cells ("bytes").
                const int xL_aln = (xL & ~3);
                const int left_cell = (xL_aln >> 2);
                const int right_cell = ((xR - 1) >> 2);
                const int cells = right_cell - left_cell; // 0,1,2,...

                uint8_t * dst = to + xL_aln;

                if (cells < 0)
                {
                    // nothing to draw on this scanline
                }
                else if (cells == 0)
                {
                    // Single cell: masked with start & end; color from LEFT endpoint
                    const uint8_t c = sample_bx(bx_from_txy((uint32_t)txx2, (uint32_t)txy2));
                    const uint8_t mL = start_mask4((unsigned)xL & 3);
                    const uint8_t mR = end_mask4((unsigned)(xR - 1) & 3);
                    poke4_mask(dst, (uint8_t)c, (uint8_t)(mL & mR));
                }
                else if (cells == 1)
                {
                    // Exactly two cells: mask both edges (no full overwrite)
                    const uint8_t cL = sample_bx(bx_from_txy((uint32_t)txx2, (uint32_t)txy2));
                    const uint8_t cR = sample_bx(bx_from_txy((uint32_t)txx1, (uint32_t)txy1));
                    const uint8_t mL = start_mask4((unsigned)xL & 3);
                    const uint8_t mR = end_mask4((unsigned)(xR - 1) & 3);
                    poke4_mask(dst, cL, mL);
                    poke4_mask(dst + 4, cR, mR);
                }
                else
                {
                    // =3 cells:
                    //  - left edge masked
                    //  - interior full cells (keep your LEFT-anchored linear DDA that was fine)
                    //  - right edge masked

                    // Left edge (from LEFT endpoint)
                    const uint8_t cL = sample_bx(bx_from_txy((uint32_t)txx2, (uint32_t)txy2));
                    const uint8_t mL = start_mask4((unsigned)xL & 3);
                    poke4_mask(dst, cL, mL);

                    // Interior cells: i = 1..cells-1 (full 4-byte writes)
                    // Interior DDA: LEFT-anchored, denom = cells (or cells-1;
                    // We draw only the interior.
                    const int interior = cells - 1;
                    if (interior > 0)
                    {
                        const int32_t dX = (int32_t)(txx1 - txx2);
                        const int32_t dY = (int32_t)(txy1 - txy2);
                        const int32_t stepX = dX / cells; // left-anchored;
                        const int32_t stepY = dY / cells;

                        int32_t tx = txx2;
                        int32_t lty = txy2;
                        for (int i = 1; i <= interior; ++i)
                        {
                            tx += stepX;
                            lty += stepY;
                            poke4_full(dst + 4 * i, sample_bx(bx_from_txy((uint32_t)tx, (uint32_t)lty)));
                        }
                    }

                    // Right edge (from RIGHT endpoint), masked
                    const uint8_t cR = sample_bx(bx_from_txy((uint32_t)txx1, (uint32_t)txy1));
                    const uint8_t mR = end_mask4((unsigned)(xR - 1) & 3);
                    poke4_mask(dst + 4 * cells, cR, mR);
                }
            }

            to += 640;
            yy += 1;
            ctau += 2;

            xx1 += ax1;
            xx2 += ax2;
            txy1 += tay1;
            txx1 += tax1;
            txy2 += tay2;
            txx2 += tax2;
        }
    }

    void do_poly(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, int color, int dd)
    {
        int a, n = 0, m, s1, s2, d1, d2, dx1, dy1, dx2, dy2;

        struct points
        {
            int x, y;
        } pnts[4], txt[4] = { { 64, 4 }, { 190, 4 }, { 190, 60 }, { 64, 60 } };

        dd = (dd + 1) & 63;

        pnts[0].x = x1;
        pnts[0].y = y1;
        pnts[1].x = x2;
        pnts[1].y = y2;
        pnts[2].x = x3;
        pnts[2].y = y3;
        pnts[3].x = x4;
        pnts[3].y = y4;

        for (n = 0, a = 1; a < 4; a++)
            if (pnts[a].y < pnts[n].y) n = a;

        s1 = n;
        s2 = n;
        d1 = (s1 + 1) & 3;
        d2 = (s2 - 1) & 3;
        dx1 = pnts[d1].x - pnts[s1].x;
        dy1 = pnts[d1].y - pnts[s1].y;
        if (dy1 == 0) dy1++;
        ax1 = 65536L * dx1 / dy1;
        xx1 = ((int32_t)pnts[s1].x << 16) + 0x8000L;
        txx1 = ((int32_t)txt[s1].x << 16) + 0x8000L;
        txy1 = ((int32_t)txt[s1].y << 16) + 0x8000L;
        tax1 = 65536L * (txt[d1].x - txt[s1].x) / dy1;
        tay1 = 65536L * (txt[d1].y - txt[s1].y) / dy1;

        dx2 = pnts[d2].x - pnts[s2].x;
        dy2 = pnts[d2].y - pnts[s2].y;
        if (dy2 == 0) dy2++;
        ax2 = 65536L * dx2 / dy2;
        xx2 = ((int32_t)pnts[s2].x << 16) + 0x8000L;
        txx2 = ((int32_t)txt[s2].x << 16) + 0x8000L;
        txy2 = ((int32_t)txt[s2].y << 16) + 0x8000L;
        tax2 = 65536L * (txt[d2].x - txt[s2].x) / dy2;
        tay2 = 65536L * (txt[d2].y - txt[s2].y) / dy2;

        yy = static_cast<short>((int32_t)pnts[s1].y);

        from = (unsigned char *)kuvataus[color];
        to = (unsigned char *)(vmem[yy]);

        dseg = (const uint8_t *)(disttaus[color] + dd * 16 * 16);
        ctau = (short *)&clrtau[clrptr][yy];

        for (n = 0; n < 4;)
        {
            if (pnts[d1].y < pnts[d2].y)
                m = pnts[d1].y;
            else
                m = pnts[d2].y;
            do_block(static_cast<short>(m - yy));

            yy = static_cast<short>(m);

            if (pnts[d1].y == pnts[d2].y)
            {
                s1 = d1;
                d1 = (s1 + 1) & 3;
                s2 = d2;
                d2 = (s2 - 1) & 3;
                n += 2;

                dx1 = pnts[d1].x - pnts[s1].x;
                dy1 = pnts[d1].y - pnts[s1].y;
                if (dy1 == 0) dy1++;
                ax1 = 65536L * dx1 / dy1;
                xx1 = ((int32_t)pnts[s1].x << 16) + 0x8000L;
                txx1 = ((int32_t)txt[s1].x << 16) + 0x8000L;
                txy1 = ((int32_t)txt[s1].y << 16) + 0x8000L;
                tax1 = 65536L * (txt[d1].x - txt[s1].x) / dy1;
                tay1 = 65536L * (txt[d1].y - txt[s1].y) / dy1;

                dx2 = pnts[d2].x - pnts[s2].x;
                dy2 = pnts[d2].y - pnts[s2].y;
                if (dy2 == 0) dy2++;
                ax2 = 65536L * dx2 / dy2;
                xx2 = ((int32_t)pnts[s2].x << 16) + 0x8000L;
                txx2 = ((int32_t)txt[s2].x << 16) + 0x8000L;
                txy2 = ((int32_t)txt[s2].y << 16) + 0x8000L;
                tax2 = 65536L * (txt[d2].x - txt[s2].x) / dy2;
                tay2 = 65536L * (txt[d2].y - txt[s2].y) / dy2;
            }
            else if (pnts[d1].y < pnts[d2].y)
            {
                s1 = d1;
                d1 = (s1 + 1) & 3;
                n++;
                dx1 = pnts[d1].x - pnts[s1].x;
                dy1 = pnts[d1].y - pnts[s1].y;
                if (dy1 == 0) dy1++;
                ax1 = 65536L * dx1 / dy1;
                xx1 = ((int32_t)pnts[s1].x << 16) + 0x8000L;
                txx1 = ((int32_t)txt[s1].x << 16) + 0x8000L;
                txy1 = ((int32_t)txt[s1].y << 16) + 0x8000L;
                tax1 = 65536L * (txt[d1].x - txt[s1].x) / dy1;
                tay1 = 65536L * (txt[d1].y - txt[s1].y) / dy1;
            }
            else
            {
                s2 = d2;
                d2 = (s2 - 1) & 3;
                n++;
                dx2 = pnts[d2].x - pnts[s2].x;
                dy2 = pnts[d2].y - pnts[s2].y;
                if (dy2 == 0) dy2++;
                ax2 = 65536L * dx2 / dy2;
                xx2 = ((int32_t)pnts[s2].x << 16) + 0x8000L;
                txx2 = ((int32_t)txt[s2].x << 16) + 0x8000L;
                txy2 = ((int32_t)txt[s2].y << 16) + 0x8000L;
                tax2 = 65536L * (txt[d2].x - txt[s2].x) / dy2;
                tay2 = 65536L * (txt[d2].y - txt[s2].y) / dy2;
            }
        }
    }

    void do_clear(uint8_t * mem, uint16_t * otau, const uint16_t * ntau)
    {
        uint16_t ycnt = 134;
        do
        {
            if (otau[0] != 640)
            {
                if (ntau[0] >= otau[0])
                {
                    uint16_t len = (uint16_t)(ntau[0] - otau[0]);
                    if (len) memset(mem + otau[0], 0, len);
                }

                if (otau[1] >= ntau[1])
                {
                    uint16_t a = (uint16_t)(ntau[1] + 1u);
                    uint16_t b = (uint16_t)(otau[1] + 1u);
                    uint16_t len = (uint16_t)(b - a);
                    if (len) memset(mem + a, 0, len);
                }
            }

            otau[0] = 640;
            otau[1] = 0;
            mem += 640;
            otau += 2;
            ntau += 2;
        } while (ycnt-- != 0);
    }

    void clear()
    {
        short *otau = (short *)clrtau[(clrptr - 1) & 7], *ntau = (short *)clrtau[clrptr];

        clrptr = (clrptr + 1) & 7;

        do_clear((uint8_t *)vmem[0], (uint16_t *)otau, (uint16_t *)ntau);
    }

    void draw()
    {
        for (int a = 0; a < plz_polys; a++)
        {
            int c = plz_object.pg[ptodraw[a].p].color;
            do_poly(plz_object.point[plz_object.pg[ptodraw[a].p].p1].xxx, plz_object.point[plz_object.pg[ptodraw[a].p].p1].yyy,
                    plz_object.point[plz_object.pg[ptodraw[a].p].p2].xxx, plz_object.point[plz_object.pg[ptodraw[a].p].p2].yyy,
                    plz_object.point[plz_object.pg[ptodraw[a].p].p3].xxx, plz_object.point[plz_object.pg[ptodraw[a].p].p3].yyy,
                    plz_object.point[plz_object.pg[ptodraw[a].p].p4].xxx, plz_object.point[plz_object.pg[ptodraw[a].p].p4].yyy, c, frames & 63);
        }
    }

    void calculate()
    {
        getspl(4 * 256 + frames * 4);

        kx = kx & 1023;
        ky = ky & 1023;
        kz = kz & 1023;
        ls_kx = ls_kx & 1023;
        ls_ky = ls_ky & 1023;

        ls_y = kosinit[ls_kx] >> 8;
        ls_x = (sinit[ls_kx] >> 8) * (sinit[ls_ky] >> 8) >> 7;
        ls_z = (sinit[ls_kx] >> 8) * (kosinit[ls_ky] >> 8) >> 7;

        count_const();
        plz_rotate();
        sort_faces();
    }

    void vect()
    {
        demo_changemode(SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, 0.0f);

        if (!Shim::isDemoFirstPart())
        {
            while (Music::getPlusFlags() < 13 && !demo_wantstoquit())
            {
                AudioPlayer::Update(true);
            }

            Common::frame_count = 0;
        }

        while (!demo_wantstoquit())
        {
            int a = Music::getPlusFlags();
            if (a >= -4 && a < 0) break;
            swappage();
            frames += demo_vsync();
            Common::frame_count = 0;
            Common::cop_pal = fpal;
            Common::do_pal = 1;

            Common::copper2();
            Common::copper3();

            calculate();
            draw();

            char * src = vram_half + 160;
            char * dst = (char *)Shim::vram;
            for (int i = 0; i < 134; i++)
            {
                memcpy(dst, src, SCREEN_WIDTH);
                dst += SCREEN_WIDTH;
                memcpy(dst, src, SCREEN_WIDTH);
                dst += SCREEN_WIDTH;
                memcpy(dst, src, SCREEN_WIDTH);
                dst += SCREEN_WIDTH;
                src += 640;
            }

            demo_blit();
            clear();
        }
    }

    void initvect()
    {
        int a, x, y, s;

        for (a = 0; a < 1524; a++)
        {
            sini[a] = s = static_cast<int>(sin(a / 1024.0 * F_PI * 4) * 127);
            s -= sini[a];
        }

        for (a = 1; a < 32; a++) // must-sini-valk
        {
            plz_pal[0 * 192 + a * 3] = 0;
            plz_pal[0 * 192 + a * 3 + 1] = 0;
            plz_pal[0 * 192 + a * 3 + 2] = static_cast<char>(a * 2);
        }
        for (a = 0; a < 32; a++)
        {
            plz_pal[0 * 192 + a * 3 + 32 * 3] = static_cast<char>(a * 2);
            plz_pal[0 * 192 + a * 3 + 1 + 32 * 3] = static_cast<char>(a * 2);
            plz_pal[0 * 192 + a * 3 + 2 + 32 * 3] = 63;
        }

        for (a = 0; a < 32; a++) // must-pun-kelt
        {
            plz_pal[1 * 192 + a * 3] = static_cast<char>(a * 2);
            plz_pal[1 * 192 + a * 3 + 1] = 0;
            plz_pal[1 * 192 + a * 3 + 2] = 0;
        }
        for (a = 0; a < 32; a++)
        {
            plz_pal[1 * 192 + a * 3 + 32 * 3] = 63;
            plz_pal[1 * 192 + a * 3 + 1 + 32 * 3] = static_cast<char>(a * 2);
            plz_pal[1 * 192 + a * 3 + 2 + 32 * 3] = 0;
        }

        for (a = 0; a < 32; a++) // must-orans-viol
        {
            plz_pal[2 * 192 + a * 3] = static_cast<char>(a);
            plz_pal[2 * 192 + a * 3 + 1] = 0;
            plz_pal[2 * 192 + a * 3 + 2] = static_cast<char>(a * 2 / 3);
        }
        for (a = 0; a < 32; a++)
        {
            plz_pal[2 * 192 + a * 3 + 32 * 3] = static_cast<char>(31 - a);
            plz_pal[2 * 192 + a * 3 + 1 + 32 * 3] = static_cast<char>(a * 2);
            plz_pal[2 * 192 + a * 3 + 2 + 32 * 3] = 21;
        }

        for (y = 0; y < 64; y++)
            for (x = 0; x < 256; x++)
            {
                kuva1[y][x] = static_cast<char>(sini[(y * 4 + sini[x * 2]) & 511] / 4 + 32);
                kuva2[y][x] = static_cast<char>(sini[(y * 4 + sini[x * 2]) & 511] / 4 + 32 + 64);
                kuva3[y][x] = static_cast<char>(sini[(y * 4 + sini[x * 2]) & 511] / 4 + 32 + 128);
            }

        for (y = 0; y < 128; y++)
            for (x = 0; x < 256; x++)
                dist1[y][x] = static_cast<char>(sini[y * 8] / 3);

        for (a = 0; a < 8 * 256; a++)
        {
            clrtau[0][a][0] = 640;
            clrtau[0][a][1] = 0;
        }
    }

    void reset()
    {
        to = nullptr;
        from = nullptr;
        ctau = nullptr;
        dseg = nullptr;

        yy = 0;
        ax1 = 0, ax2 = 0, xx1 = 0, xx2 = 0;
        txx1 = 0, txy1 = 0, tax1 = 0, tay1 = 0;
        txx2 = 0, txy2 = 0, tax2 = 0, tay2 = 0;

        CLEAR(sini);
        CLEAR(vram_half);

        CLEAR(kuva1);
        CLEAR(kuva2);
        CLEAR(kuva3);
        CLEAR(dist1);

        CLEAR(sinx);
        CLEAR(siny);

        CLEAR(plz_pal);

        CLEAR(clrtau);
        clrptr = 0;

        CLEAR(plz_vidmem);
        CLEAR(pals);

        curpal = 0;
        ttptr = 0;

        l1 = 1000, l2 = 2000, l3 = 3000, l4 = 4000;
        k1 = 3500, k2 = 2300, k3 = 3900, k4 = 3670;

        il1 = 1000, il2 = 2000, il3 = 3000, il4 = 4000;
        ik1 = 3500, ik2 = 2300, ik3 = 3900, ik4 = 3670;

        pompi = 0;

        CLEAR(plz_fadepal);

        drop_y = 0;

        CLEAR(LC1);
        CLEAR(LC2);
        CLEAR(LC3);
        CLEAR(LC4);

        CLEAR(fpal);

        plz_polys = 0;
        CLEAR(light_src);
        CLEAR(lls);

        cxx = {}, cxy = {}, cxz = {}, cyx = {}, cyy = {}, cyz = {}, czx = {}, czy = {}, czz = {};
        kx = 0, ky = 0, kz = 0, dis = 320, ltx = 0, ty = -50;
        ls_kx = 0, ls_ky = 0, ls_kz = 0, ls_x = 0, ls_y = 0, ls_z = 128;
        page = 0;
        frames = 0;

        CLEAR(ptodraw);
    }

    void main()
    {
        reset();

        Shim::clearScreen();

        initvect();
        plz();
        vect();

        demo_changemode(SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Default_JSSS);
    }
}
