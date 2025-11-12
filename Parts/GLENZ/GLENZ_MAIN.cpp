#include "Parts/Common.h"

#include "GLENZ_DATA.h"

namespace Glenz
{
    constexpr const size_t NROWS = 256;
    constexpr const size_t ROWS_ACTIVE = 200;
    constexpr const size_t MAXLINES = 4096;

    extern void demo_glz(uint16_t ax, uint16_t dx, uint8_t * ebx_minus2);
    extern void demo_glz2(uint16_t ax, uint16_t dx, uint8_t * ebx_minus2);

    namespace
    {
        unsigned char backpal[16 * 3] = {
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        };

        char lightshift = 0;

        void (*demomode[3])(uint16_t, uint16_t, uint8_t *) = { demo_glz, demo_glz, demo_glz2 };

        int32_t projxmul = 0;
        int32_t projymul = 0;
        uint16_t projxadd = 0;
        uint16_t projyadd = 0;
        int32_t projminz = 0;
        uint16_t projminzshr = 0;

        int16_t wminx = 0;
        int16_t wminy = 0;
        int16_t wmaxx = 100;
        int16_t wmaxy = 100;

        uint16_t count = 0;

        uintptr_t cntoff = 0;

        uintptr_t pointsoff = 0;

        char bgpic[65535];
        char * fcrow[100];
        char * fcrow2[16];

        char pal1[PaletteByteCount];
        char pal2[PaletteByteCount];

        int32_t points2[256]{};
        int32_t points2b[256]{};
        int points3[256 * 2]{};

        int edges2[64]{};

        short polylist[256]{};
        short matrix[9]{};

        char pal[PaletteByteCount]{};
        char tmppal[PaletteByteCount]{};

        int repeat{};
        int frame{};

        uint16_t glenz_rows[512] = { 0 };

        /* Edge layout (byte offsets) */
        enum
        {
            NE_X = 0,
            NE_Y1 = 4,
            NE_Y2 = 6,
            NE_COLOR = 8,
            NE_NEXT = 10,
            NE_DX = 14,
            NE_STRIDE = 18
        };

        uint8_t newdata1[0x10000]{}; /* two 32 KiB pages */
        uint32_t ndp0 = 0;           /* 0 or 0x8000: which page is “new” */
        uint32_t ndp = 0;            /* write cursor within newdata1 */

        uint32_t nep[NROWS] = { 0 }; /* buckets by row (byte offsets into ne[]) */
        uint32_t nl[8192] = { 0 };   /* active edge list per row (enough capacity) */
        uint32_t nec = NE_STRIDE;    /* next-free offset in ne[] */
        uint8_t ne[MAXLINES * NE_STRIDE] = { 0 };

        uint32_t yrow = 0, yrowad = 0;

        uint32_t tmp_firstvx{};
        uint16_t tmp_color{};
        uint32_t siend{};
        uint8_t rolcol[256]{};
        uint8_t rolused[256]{};

        /* ---- carry flag emulation (set by checkhiddenbx_det) ---- */
        uint8_t g_cf = 0; /* 1 = hidden/backface, 0 = visible */

        int32_t g_xadd = 0, g_yadd = 0, g_zadd = 0; /* _xadd/_yadd/_zadd in ASM */
        int32_t g_m[9] = { 0 };
    }

    inline uint8_t clamp63(uint32_t v)
    {
        return (uint8_t)(v > 63u ? 63u : v);
    }

    // Write one RGB triple (0..63 each) to DAC data port (0x3C9)
    static inline void setpalxxx(uint8_t r, uint8_t g, uint8_t b)
    {
        if (r > 63u) r = 63u;
        if (g > 63u) g = 63u;
        if (b > 63u) b = 63u;
        Shim::outp(0x03C9u, r);
        Shim::outp(0x03C9u, g);
        Shim::outp(0x03C9u, b);
    }

    /* Compute the same signed det as ASM and set CF identically. Return det. */
    static inline int32_t checkhiddenbx_det(const uint16_t * v)
    {
        int32_t x0 = (int16_t)v[0], y0 = (int16_t)v[1];
        int32_t x1 = (int16_t)v[2], y1 = (int16_t)v[3];
        int32_t x2 = (int16_t)v[4], y2 = (int16_t)v[5];
        int64_t a = (int64_t)(x0 - x1) * (int64_t)(y0 - y2);
        int64_t b = (int64_t)(y0 - y1) * (int64_t)(x0 - x2);
        int64_t det = a - b;

        g_cf = (det < 0); /* CF set if det<0 */

        return (int32_t)det;
    }

    /* Copy [X,Y] dword from points (12-byte stride) if idx != last; advance DI only if emitted. */
    static inline void adddot_emit(uint8_t ** pdi, uint16_t idx, uint16_t * last_idx)
    {
        if (idx == *last_idx) return;

        *last_idx = idx;
        const uint8_t * base = (const uint8_t *)(uintptr_t)pointsoff + (uint32_t)idx * 12u;
        *(uint32_t *)(*pdi) = *(const uint32_t *)base; /* X(low16),Y(high16) */

        *pdi += 4;
    }

    void demo_glz2(uint16_t ax, uint16_t dx, uint8_t * ebx_minus2)
    {
        (void)ax;
        (void)dx;

        if (g_cf) return; /* hidden => no change */

        uint8_t al = *ebx_minus2;
        *ebx_minus2 = (uint8_t)((al >> 1) & 1u); /* byte write */
    }

    void demo_glz(uint16_t ax, uint16_t dx, uint8_t * ebx_minus2)
    {
        if (g_cf)
        {
            uint16_t w = *(uint16_t *)ebx_minus2;
            uint8_t id = (uint8_t)w;
            uint8_t slot = rolcol[id];
            rolcol[id] = 0;
            if (slot) rolused[slot] = 0;
            uint8_t low = (uint8_t)(((id >> 1) & 1u) << 2);
            *(uint16_t *)ebx_minus2 = (uint16_t)((w & 0xFF00u) | low);

            return;
        }

        uint32_t cat = ((uint32_t)dx << 16) | (uint32_t)ax;
        uint16_t inten;

        if ((uint8_t)lightshift != 9)
        {
            uint16_t t1 = (uint16_t)((cat >> 8) & 0xFFFFu);
            uint32_t cat2 = ((uint32_t)dx << 16) | (uint32_t)t1;
            uint16_t t2 = (uint16_t)((cat2 >> 1) & 0xFFFFu);
            inten = (uint16_t)(t1 + t2);
        }
        else
        {
            inten = (uint16_t)((cat >> 7) & 0xFFFFu);
        }

        if (inten > 63) inten = 63;

        uint8_t ah = (uint8_t)inten;

        uint16_t w = *(uint16_t *)ebx_minus2;
        uint16_t bp = w;
        uint8_t id = (uint8_t)w;
        uint8_t slot = rolcol[id];

        if (slot == 0)
        {
            uint8_t cand = 2;
            for (int i = 0; i < 15; ++i)
            {
                if (!rolused[cand])
                {
                    slot = cand;
                    break;
                }
                cand = (uint8_t)(cand + 2);
            }
            if (slot == 0) slot = 2;
            rolcol[id] = slot;
            rolused[slot] = 1;
        }

        /* Low byte := (slot<<3); then program DAC palette block starting at that index */
        *(uint8_t *)ebx_minus2 = (uint8_t)(slot << 3);
        Shim::outp(0x03C8u, (uint8_t)(slot << 3)); /* set DAC index */

        uint8_t r0, g0, b0;

        if (bp & 2)
        {
            b0 = ah;
            g0 = (uint8_t)(ah >> 1);
            r0 = 0;
        } /* blue-ish */
        else
        {
            r0 = g0 = b0 = ah;
        } /* grayscale */

        for (int i = 0; i < 16; ++i)
        {
            uint8_t ra = (uint8_t)(backpal[i * 3 + 0] >> 2);
            uint8_t ga = (uint8_t)(backpal[i * 3 + 1] >> 2);
            uint8_t ba = (uint8_t)(backpal[i * 3 + 2] >> 2);
            setpalxxx((uint8_t)clamp63((uint32_t)r0 + ra), (uint8_t)clamp63((uint32_t)g0 + ga), (uint8_t)clamp63((uint32_t)b0 + ba));
        }
    }

    int ceasypolylist(short * ppolylist, const unsigned short * polys, const int32_t * ppoints3)
    {
        /* points base = points3 + 4 (skip header) */
        pointsoff = (uintptr_t)((uint8_t *)ppoints3 + 4);

        uint8_t * di = (uint8_t *)ppolylist;

        for (;;)
        {
            uint16_t sides = *polys++;
            if (sides == 0)
            {
                *(uint16_t *)di = 0;
                return 0;
            }

            di += 2;
            *(uint16_t *)di = *polys++;
            di += 2; /* reserve count; copy visibility */
            uint8_t * cnt_ptr = di;
            cntoff = (uintptr_t)cnt_ptr;

            uint16_t last_idx = 0xFFFF;
            for (uint16_t c = sides; c--;)
            {
                uint16_t idx = *polys++;
                adddot_emit(&di, idx, &last_idx); /* advance only if emitted */
            }

            if (di - cnt_ptr >= 4 && *(uint32_t *)cnt_ptr == *(uint32_t *)(di - 4)) di -= 4;

            uint16_t dwords = (uint16_t)(((uintptr_t)di - (uintptr_t)cnt_ptr) >> 2);
            *(uint16_t *)(cnt_ptr - 4) = dwords; /* count at [cnt_ptr-4] */

            /* Compute same det, set CF, and pass DX:AX as in ASM */
            int32_t det = checkhiddenbx_det((const uint16_t *)cnt_ptr);
            uint16_t ax = (uint16_t)(det & 0xFFFFu);
            uint16_t dx = (uint16_t)((uint32_t)det >> 16);

            demomode[0](ax, dx, (uint8_t *)(cnt_ptr - 2));
        }
    }

    static inline void ne_write32(uint32_t p, uint32_t v)
    {
        *(uint32_t *)&ne[p] = v;
    }

    static inline void ne_write16(uint32_t p, uint16_t v)
    {
        *(uint16_t *)&ne[p] = v;
    }

    static inline uint32_t ne_read32(uint32_t p)
    {
        return *(uint32_t *)&ne[p];
    }

    static inline uint16_t ne_read16(uint32_t p)
    {
        return *(uint16_t *)&ne[p];
    }

    static inline void ensure_seed_if_empty(uint32_t base)
    {
        uint32_t dw = *(uint32_t *)&newdata1[base];
        if (dw == 0)
        {
            *(uint16_t *)&newdata1[base + 0] = 0xFFFF;
            *(uint16_t *)&newdata1[base + 2] = 0;
        }
    }

    static void ng_init()
    {
        ndp0 ^= 0x8000u;
        ndp0 &= 0x8000u;
        ndp = ndp0;

        nec = NE_STRIDE;
        memset(nep, 0, sizeof(nep));

        ensure_seed_if_empty(0x0000);
        ensure_seed_if_empty(0x8000);

        yrow = 0;
        yrowad = 0;
    }

    static uint32_t add_edge_if_visible(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
    {
        if (y1 > y2)
        {
            int16_t tx = x1;
            x1 = x2;
            x2 = tx;
            int16_t ty = y1;
            y1 = y2;
            y2 = ty;
        }

        if (y1 == y2) return 0;

        int32_t num = ((int32_t)x2 - (int32_t)x1) << 16;
        int32_t den = (int32_t)y2 - (int32_t)y1;
        if (!den) return 0;
        int32_t dx = num / den;

        int32_t x_fixed = ((int32_t)x1) << 16;

        if (y1 < 0)
        {
            if (y2 <= 0) return 0;
            x_fixed += dx * (int32_t)(-y1);
            y1 = 0;
        }

        if (y1 > 199) return 0;

        if (nec + NE_STRIDE > sizeof(ne)) return 0;
        uint32_t e = nec;
        nec += NE_STRIDE;

        ne_write32(e + NE_X, (uint32_t)x_fixed);
        ne_write16(e + NE_Y1, (uint16_t)y1);
        ne_write16(e + NE_Y2, (uint16_t)y2);
        ne_write16(e + NE_COLOR, color);
        ne_write32(e + NE_DX, (uint32_t)dx);

        uint32_t head = nep[y1];
        if (!head)
        {
            nep[y1] = e;
            ne_write32(e + NE_NEXT, 0);
            return e;
        }

        /* Coalesce duplicate (same Y2, X, DX) by XOR color; else push to list tail */
        uint32_t it = head, prev = 0;

        while (it)
        {
            if (ne_read16(e + NE_Y2) == ne_read16(it + NE_Y2) && ne_read32(e + NE_X) == ne_read32(it + NE_X) && ne_read32(e + NE_DX) == ne_read32(it + NE_DX))
            {
                uint16_t old = ne_read16(it + NE_COLOR);
                uint16_t merged = (old & 0xFF00u) | ((old ^ color) & 0x00FFu);
                ne_write16(it + NE_COLOR, merged);

                nec -= NE_STRIDE;
                return 0;
            }
            prev = it;
            it = ne_read32(it + NE_NEXT);
        }
        ne_write32(prev + NE_NEXT, e);
        ne_write32(e + NE_NEXT, 0);
        return e;
    }

    static void ng_pass2()
    {
        uint32_t * nl_head = nl;
        uint32_t list_end_off = 0;

        yrow = 0;
        yrowad = 0;

        for (uint32_t row = 0; row < ROWS_ACTIVE; ++row)
        {
            /* append new edges for this row */
            for (uint32_t e = nep[row]; e; e = ne_read32(e + NE_NEXT))
            {
                *(uint32_t *)((uint8_t *)nl_head + list_end_off) = e;
                list_end_off += 4;
            }

            /* insertion sort by current X (16.16) */
            if (list_end_off > 4)
            {
                uint8_t * base = (uint8_t *)nl_head;
                for (uint32_t off = 4; off < list_end_off; off += 4)
                {
                    uint32_t cur = *(uint32_t *)(base + off);
                    int32_t x = (int32_t)ne_read32(cur + NE_X);
                    int32_t j = (int32_t)off - 4;
                    while (j >= 0)
                    {
                        uint32_t prev = *(uint32_t *)(base + j);
                        if (x >= (int32_t)ne_read32(prev + NE_X)) break;
                        *(uint32_t *)(base + j + 4) = prev;
                        j -= 4;
                    }
                    *(uint32_t *)(base + j + 4) = cur;
                }
            }

            const uint32_t ndp_scanline_start = ndp;
            uint32_t lsiend = list_end_off;
            uint32_t ebp_ndp = ndp;
            uint16_t cx_last = 0;

            uint8_t * base = (uint8_t *)nl_head;
            uint32_t write_off = 0;

            for (uint32_t off = 0; off < lsiend; off += 4)
            {
                uint32_t e = *(uint32_t *)(base + off);

                int16_t y2 = (int16_t)ne_read16(e + NE_Y2);
                if ((int32_t)(int16_t)yrow >= (int32_t)y2) continue;

                *(uint32_t *)(base + write_off) = e;
                write_off += 4;

                int32_t x_fixed = (int32_t)ne_read32(e + NE_X);
                int32_t dx = (int32_t)ne_read32(e + NE_DX);

                int16_t xi = (int16_t)(x_fixed >> 16);
                if (xi > (SCREEN_WIDTH - 1)) xi = (SCREEN_WIDTH - 1);
                if (xi < 1) xi = 1;

                if (ebp_ndp > ndp_scanline_start && cx_last == (uint16_t)xi)
                {
                    *(uint16_t *)&newdata1[ebp_ndp - 2] ^= ne_read16(e + NE_COLOR);
                }
                else
                {
                    uint16_t pos = (uint16_t)((uint16_t)xi + (uint16_t)yrowad);
                    *(uint16_t *)&newdata1[ebp_ndp + 0] = pos;
                    *(uint16_t *)&newdata1[ebp_ndp + 2] = ne_read16(e + NE_COLOR);
                    ebp_ndp += 4;
                    cx_last = (uint16_t)xi;
                }

                /* advance for next row (emit-then-advance order) */
                ne_write32(e + NE_X, (uint32_t)(x_fixed + dx));
            }

            ndp = ebp_ndp;
            list_end_off = write_off;

            yrow += 1;
            yrowad += SCREEN_WIDTH;
        }

        /* end markers: [63999,0], [-1,0] */
        *(uint16_t *)&newdata1[ndp + 0] = 63999;
        *(uint16_t *)&newdata1[ndp + 2] = 0;
        ndp += 4;

        *(uint16_t *)&newdata1[ndp + 0] = 0xFFFF;
        *(uint16_t *)&newdata1[ndp + 2] = 0;
        ndp += 4;
    }

    static void ng_pass3()
    {
        unsigned short base{};
        unsigned int newp{}, lastp{};
        unsigned int ecx{}, edx{};
        unsigned short di{}; /* VRAM cursor 0..63999 */
        unsigned char al{};  /* NEW  color toggle */
        unsigned char ah{};  /* LAST color toggle */

        /* Force ndp0 into a 16-bit page offset (handles signed ndp0 safely). */
        base = (unsigned short)ndp0;           /* 0..65535 */
        newp = (unsigned int)(base & 0x8000u); /* 0 or 0x8000 */
        lastp = newp ^ 0x8000u;

        /* Baseline fetch order that drew the object: LAST first, then NEW */
        edx = *(unsigned int *)&newdata1[lastp];
        lastp += 4; /* LAST stream dword  */
        ecx = *(unsigned int *)&newdata1[newp];
        newp += 4; /* NEW  stream dword  */

        for (;;)
        {
            unsigned short cx = (unsigned short)ecx; /* NEW  pos (low word)  */
            unsigned short dx = (unsigned short)edx; /* LAST pos (low word)  */

            if (dx < cx)
            {
                if (al != ah && dx > di)
                {
                    unsigned short cnt = (unsigned short)(dx - di);
                    unsigned short i, base_di = di;
                    for (i = 0; i < cnt; ++i)
                        Shim::vram[base_di + i] = (unsigned char)(al | bgpic[base_di + i]);
                }
                di = dx;

                /* toggle LAST color with DL (low byte of high 16 bits) */
                ah ^= (unsigned char)(edx >> 16);

                /* advance LAST stream */
                edx = *(unsigned int *)&newdata1[lastp];
                lastp += 4;
            }
            else if (dx == cx)
            {
                if (cx == 0xFFFFu) break;

                if (al != ah && dx > di)
                {
                    unsigned short cnt = (unsigned short)(dx - di);
                    unsigned short i, base_di = di;
                    for (i = 0; i < cnt; ++i)
                        Shim::vram[base_di + i] = (unsigned char)(al | bgpic[base_di + i]);
                }
                di = dx;

                ah ^= (unsigned char)(edx >> 16); /* DL */
                edx = *(unsigned int *)&newdata1[lastp];
                lastp += 4; /* advance LAST */
            }
            else
            { /* cx < dx ? NEW side */
                if (al != ah && cx > di)
                {
                    unsigned short cnt = (unsigned short)(cx - di);
                    unsigned short i, base_di = di;
                    for (i = 0; i < cnt; ++i)
                        Shim::vram[base_di + i] = (unsigned char)(al | bgpic[base_di + i]);
                }
                di = cx;

                al ^= (unsigned char)(ecx >> 16); /* CL */
                ecx = *(unsigned int *)&newdata1[newp];
                newp += 4; /* advance NEW */
            }
        }
    }

    void newgroup(uint16_t mode, const int16_t * pg)
    {
        if (mode == 0)
        {
            ng_init();
            return;
        }

        if (mode == 1)
        {
            while (1)
            {
                uint16_t sides = (uint16_t)pg[0];
                if (sides == 0) break;
                uint16_t color = (uint16_t)pg[1];

                const int16_t * verts = pg + 2;
                const int16_t *first = verts, *prev = verts, *cur = verts + 2;

                for (uint16_t k = 0; k < sides; ++k)
                {
                    if (k == sides - 1) cur = first;
                    add_edge_if_visible(prev[0], prev[1], cur[0], cur[1], color);
                    prev = cur;
                    cur += 2;
                }

                pg = verts + sides * 2;
            }

            return;
        }

        ng_pass2();
        ng_pass3();
    }

    void cglenzinit()
    {
        newgroup(0, nullptr);
    }

    void cglenzpolylist(short * lpolylist)
    {
        newgroup(1, lpolylist);
    }

    void cglenzdone()
    {
        newgroup(2, nullptr);
    }

    int csetmatrix(short * m9, int32_t X, int32_t Y, int32_t Z)
    {
        /* store translations (_xadd/_yadd/_zadd) */
        g_xadd = X;
        g_yadd = Y;
        g_zadd = Z;

        /* sign-extend 9 words to 32-bit, same order as ASM setmatrix stores */
        g_m[0] = (int32_t)m9[0]; /* mtrm00 */
        g_m[1] = (int32_t)m9[1]; /* mtrm02 */
        g_m[2] = (int32_t)m9[2]; /* mtrm04 */
        g_m[3] = (int32_t)m9[3]; /* mtrm06 */
        g_m[4] = (int32_t)m9[4]; /* mtrm08 */
        g_m[5] = (int32_t)m9[5]; /* mtrm10 */
        g_m[6] = (int32_t)m9[6]; /* mtrm12 */
        g_m[7] = (int32_t)m9[7]; /* mtrm14 */
        g_m[8] = (int32_t)m9[8]; /* mtrm16 */

        return 0;
    }

    static inline int32_t dot_q15(int32_t a0, int32_t a1, int32_t a2, int32_t b0, int32_t b1, int32_t b2)
    {
        /* 64-bit accumulation, then arithmetic shift by 15 */
        int64_t s = (int64_t)a0 * (int64_t)b0 + (int64_t)a1 * (int64_t)b1 + (int64_t)a2 * (int64_t)b2;

        return (int32_t)(s >> 15);
    }

    int crotlist(int32_t * dst, const int32_t * src)
    {
        uint16_t src_count = (uint16_t)(src[0] & 0xFFFFu);
        uint32_t dst_hdr = (uint32_t)dst[0];
        uint16_t dst_count = (uint16_t)(dst_hdr & 0xFFFFu);

        uint16_t new_dst_count = (uint16_t)(dst_count + src_count);
        dst[0] = (int32_t)((dst_hdr & 0xFFFF0000u) | new_dst_count);

        int32_t * out = dst + 1 + (3 * (int)dst_count);
        const int32_t * in = src + 1;

        for (uint16_t i = 0; i < src_count; ++i)
        {
            int32_t x = in[0];
            int32_t y = in[1];
            int32_t z = in[2];

            int32_t X = dot_q15(g_m[0], g_m[1], g_m[2], x, y, z) + g_xadd;

            int32_t Row1 = dot_q15(g_m[3], g_m[4], g_m[5], x, y, z); /* later = Z */
            int32_t Y = dot_q15(g_m[6], g_m[7], g_m[8], x, y, z) + g_yadd;
            int32_t Z = Row1 + g_zadd;

            out[0] = X;
            out[1] = Y;
            out[2] = Z;

            in += 3;
            out += 3;
        }

        return (int)src_count;
    }

    void cliplist(int32_t * esi)
    {
        uint16_t cx = *(const uint16_t *)((const void *)esi);
        esi = (int32_t *)((uint8_t *)esi + 4);

        while (cx--)
        {
            if (!(esi[1] < 1500))
            {
                esi[1] = 1500;
            }
            esi += 3; // advance to next (X,Y,Z)
        }
    }

    static void projlist(const int32_t * ESI /*src*/, uint16_t * EDI /*dst*/)
    {
        uint16_t cx = *(const uint16_t *)(const void *)ESI;
        ESI = (const int32_t *)((const uint8_t *)ESI + 4);
        count = cx;

        uint16_t have = *EDI;
        *EDI = (uint16_t)(have + cx);

        // compute byte offset: ax = have*12  (ax = have*3; ax <<= 2)
        uint16_t ax = have;
        uint16_t bx = have;
        ax = (uint16_t)((ax << 1) + bx); // have*3
        ax = (uint16_t)(ax << 2);        // have*12

        uint8_t * out = (uint8_t *)EDI + ax + 4;

        // loop over points
        while (count--)
        {
            int32_t X = ESI[0];
            int32_t Y = ESI[1];
            int32_t Z = ESI[2];

            uint16_t bp = 0;
            *(int32_t *)(out + 8) = Z;

            int32_t Zdiv = Z;
            if (Zdiv < projminz)
            {
                Zdiv = projminz;
                bp |= 16;
            }

            // ---- project Y ----
            int32_t qY = (int32_t)((int64_t)Y * (int64_t)projymul / (int64_t)Zdiv);
            int16_t y = (int16_t)(qY + (int32_t)projyadd);

            // clip flags for Y (compare as 16-bit like the ASMâ€™s AX)
            if (y > wmaxy) bp |= 8;
            if (y < wminy) bp |= 4;

            *(int16_t *)(out + 2) = y;

            // ---- project X ----
            int32_t qX = (int32_t)((int64_t)X * (int64_t)projxmul / (int64_t)Zdiv);
            int16_t x = (int16_t)(qX + (int32_t)projxadd);

            if (x > wmaxx) bp |= 2;
            if (x < wminx) bp |= 1;

            *(int16_t *)(out + 0) = x;

            *(uint16_t *)(out + 4) = bp;

            ESI += 3;
            out += 12;
        }
    }

    void cprojlist(uint16_t * dst, const int32_t * src)
    {
        projlist(src, dst);
    }

    static inline void setrows(uint16_t dx, uint16_t lcount)
    {
        uint16_t ax = 0;

        uint16_t * p = glenz_rows;
        for (uint16_t i = 0; i < lcount; ++i)
        {
            *p++ = ax;
            ax = (uint16_t)(ax + dx); // 16-bit wrap like ADD AX,DX
        }
    }

    void init320x200C()
    {
        projxmul = 256;
        projymul = 213;

        projxadd = 160;
        projyadd = 130;

        projminz = 128;
        projminzshr = 7;

        // wminx/y, wmaxx/y
        wminx = 0;
        wminy = 0;
        wmaxx = SCREEN_WIDTH - 1;
        wmaxy = SCREEN_HEIGHT - 1;

        setrows(/*dx=*/PLANAR_WIDTH, /*count=*/200);
    }

    static inline int16_t wrap_deg10(int16_t a)
    {
        int32_t x = a;

        while (x >= 3600)
            x -= 3600;
        while (x < 0)
            x += 3600;

        return (int16_t)x;
    }

    static inline int16_t q15_mul_exact(int16_t a, int16_t b)
    {
        int32_t p = (int32_t)a * (int32_t)b;
        uint32_t up = (uint32_t)p;
        uint32_t shr = up >> 15; // SHRD 15 of DX:AX combined
        return (int16_t)(shr & 0xFFFF);
    }

    static inline int16_t add16ForMatrix(int16_t acc, int16_t v)
    {
        return (int16_t)((int32_t)acc + (int32_t)v);
    }

    static inline int16_t sub16ForMatrix(int16_t acc, int16_t v)
    {
        return (int16_t)((int32_t)acc - (int32_t)v);
    }

    static inline void trig_deg10(int16_t a, int16_t * s, int16_t * c)
    {
        int16_t w = wrap_deg10(a);

        *s = Data::sintable16[w];
        *c = Data::costable16[w];
    }

    static void calcmatrix(int16_t rx, int16_t ry, int16_t rz, int16_t * d)
    {
        int16_t sx, cx, sy, cy, sz, cz;
        trig_deg10(rx, &sx, &cx);
        trig_deg10(ry, &sy, &cy);
        trig_deg10(rz, &sz, &cz);

        // Pre-terms
        int16_t xs_ys = q15_mul_exact(sx, sy);
        int16_t xs_yc = q15_mul_exact(sx, cy);
        int16_t xc_ys = q15_mul_exact(cx, sy);
        int16_t xc_yc = q15_mul_exact(cx, cy);

        // 0 =  Ycos*Zcos - Xsin*Ysin*Zsin
        int16_t v0 = q15_mul_exact(cy, cz);
        v0 = sub16ForMatrix(v0, q15_mul_exact(xs_ys, sz));
        d[0] = v0;

        // 2 =  Xsin*Ysin*Zcos + Ycos*Zsin
        int16_t v2 = q15_mul_exact(xs_ys, cz);
        v2 = add16ForMatrix(v2, q15_mul_exact(cy, sz));
        d[1] = v2;

        // 4 = -Xcos*Ysin
        d[2] = (int16_t)(-xc_ys);

        // 6 = -Xcos*Zsin
        d[3] = (int16_t)(-q15_mul_exact(cx, sz));

        // 8 =  Xcos*Zcos
        d[4] = q15_mul_exact(cx, cz);

        // 10 = Xsin
        d[5] = sx;

        // 12 = Xsin*Ycos*Zsin + Ysin*Zcos
        int16_t v12 = q15_mul_exact(xs_yc, sz);
        v12 = add16ForMatrix(v12, q15_mul_exact(sy, cz));
        d[6] = v12;

        // 14 = Ysin*Zsin - Xsin*Ycos*Zcos
        int16_t v14 = q15_mul_exact(sy, sz);
        v14 = sub16ForMatrix(v14, q15_mul_exact(xs_yc, cz));
        d[7] = v14;

        // 16 = Xcos*Ycos
        d[8] = xc_yc;
    }

    void cmatrix_yxzC(int16_t rx, int16_t ry, int16_t rz, int16_t * dst)
    {
        calcmatrix(/*rx*/ ry, /*ry*/ rx, /*rz*/ rz, dst);
    }

    // rX * rY * rZ
    void calcmatrix0(int16_t rx, int16_t ry, int16_t rz, int16_t * d)
    {
        int16_t sx, cx, sy, cy, sz, cz;
        trig_deg10(rx, &sx, &cx);
        trig_deg10(ry, &sy, &cy);
        trig_deg10(rz, &sz, &cz);

        int16_t xs_ys = q15_mul_exact(sx, sy);
        int16_t xc_ys = q15_mul_exact(cx, sy);

        d[0] = q15_mul_exact(cy, cz);
        d[1] = q15_mul_exact(cy, sz);
        d[2] = (int16_t)(-sy);

        int16_t r10 = q15_mul_exact(xs_ys, cz);
        r10 = sub16ForMatrix(r10, q15_mul_exact(cx, sz));
        d[3] = r10;

        int16_t r11 = q15_mul_exact(xs_ys, sz);
        r11 = add16ForMatrix(r11, q15_mul_exact(cx, cz));
        d[4] = r11;

        d[5] = q15_mul_exact(cy, sx);

        int16_t r20 = q15_mul_exact(xc_ys, cz);
        r20 = add16ForMatrix(r20, q15_mul_exact(sx, sz));
        d[6] = r20;

        int16_t r21 = q15_mul_exact(xc_ys, sz);
        r21 = sub16ForMatrix(r21, q15_mul_exact(sx, cz));
        d[7] = r21;

        d[8] = q15_mul_exact(cy, cx);
    }

    // rX, rY, rZ as three consecutive 3×3 Q15 matrices
    void calcmatrixsep(int16_t rx, int16_t ry, int16_t rz, int16_t * dst)
    {
        int16_t sx, cx, sy, cy, sz, cz;
        trig_deg10(rx, &sx, &cx);
        trig_deg10(ry, &sy, &cy);
        trig_deg10(rz, &sz, &cz);

        int16_t * m = dst;
        // rX
        m[0] = 32767;
        m[1] = 0;
        m[2] = 0;
        m[3] = 0;
        m[4] = cx;
        m[5] = sx;
        m[6] = 0;
        m[7] = (int16_t)(-sx);
        m[8] = cx;

        // rY
        m += 9;
        m[0] = cy;
        m[1] = 0;
        m[2] = (int16_t)(-sy);
        m[3] = 0;
        m[4] = 32767;
        m[5] = 0;
        m[6] = sy;
        m[7] = 0;
        m[8] = cy;

        // rZ
        m += 9;
        m[0] = cz;
        m[1] = sz;
        m[2] = 0;
        m[3] = (int16_t)(-sz);
        m[4] = cz;
        m[5] = 0;
        m[6] = 0;
        m[7] = 0;
        m[8] = 32767;
    }

    // In-place 3×3 multiply: m1 = m1 * m2 (Q15), 16-bit wrapped accumulation
    void mulmatrices(int16_t * m1, const int16_t * m2)
    {
        int16_t a00 = m1[0], a01 = m1[1], a02 = m1[2];
        int16_t a10 = m1[3], a11 = m1[4], a12 = m1[5];
        int16_t a20 = m1[6], a21 = m1[7], a22 = m1[8];

        int16_t b00 = m2[0], b01 = m2[1], b02 = m2[2];
        int16_t b10 = m2[3], b11 = m2[4], b12 = m2[5];
        int16_t b20 = m2[6], b21 = m2[7], b22 = m2[8];

        int16_t r00 = q15_mul_exact(a00, b00);
        r00 = add16ForMatrix(r00, q15_mul_exact(a01, b10));
        r00 = add16ForMatrix(r00, q15_mul_exact(a02, b20));

        int16_t r01 = q15_mul_exact(a00, b01);
        r01 = add16ForMatrix(r01, q15_mul_exact(a01, b11));
        r01 = add16ForMatrix(r01, q15_mul_exact(a02, b21));

        int16_t r02 = q15_mul_exact(a00, b02);
        r02 = add16ForMatrix(r02, q15_mul_exact(a01, b12));
        r02 = add16ForMatrix(r02, q15_mul_exact(a02, b22));

        int16_t r10 = q15_mul_exact(a10, b00);
        r10 = add16ForMatrix(r10, q15_mul_exact(a11, b10));
        r10 = add16ForMatrix(r10, q15_mul_exact(a12, b20));

        int16_t r11 = q15_mul_exact(a10, b01);
        r11 = add16ForMatrix(r11, q15_mul_exact(a11, b11));
        r11 = add16ForMatrix(r11, q15_mul_exact(a12, b21));

        int16_t r12 = q15_mul_exact(a10, b02);
        r12 = add16ForMatrix(r12, q15_mul_exact(a11, b12));
        r12 = add16ForMatrix(r12, q15_mul_exact(a12, b22));

        int16_t r20 = q15_mul_exact(a20, b00);
        r20 = add16ForMatrix(r20, q15_mul_exact(a21, b10));
        r20 = add16ForMatrix(r20, q15_mul_exact(a22, b20));

        int16_t r21 = q15_mul_exact(a20, b01);
        r21 = add16ForMatrix(r21, q15_mul_exact(a21, b11));
        r21 = add16ForMatrix(r21, q15_mul_exact(a22, b21));

        int16_t r22 = q15_mul_exact(a20, b02);
        r22 = add16ForMatrix(r22, q15_mul_exact(a21, b12));
        r22 = add16ForMatrix(r22, q15_mul_exact(a22, b22));

        m1[0] = r00;
        m1[1] = r01;
        m1[2] = r02;
        m1[3] = r10;
        m1[4] = r11;
        m1[5] = r12;
        m1[6] = r20;
        m1[7] = r21;
        m1[8] = r22;
    }

    void zoomer2()
    {
        int a{}, b{}, c{}, y{};
        char * v{};
        int framez2 = 0;
        int zly{}, zy{}, zya{};
        int zly2{}, zy2{};

        Shim::outp(0x3c7, 0);

        for (a = 0; a < static_cast<int>(PaletteByteCount); a++)
            pal1[a] = Shim::inp(0x3c9);

        zy = 0;
        zya = 0;
        zly = 0;
        zy2 = 0;
        zly2 = 0;
        framez2 = 0;

        while (!demo_wantstoquit())
        {
            if (zy == 260) break;
            zly = zy;
            zya++;
            zy += zya / 4;
            if (zy > 260) zy = 260;
            v = (char *)(Shim::vram + zly * PLANAR_WIDTH * 4);
            for (y = zly; y <= zy; y++)
            {
                memset(v, 255, PLANAR_WIDTH * 4);
                v += PLANAR_WIDTH * 4;
            }
            zly2 = zy2;
            zy2 = 125 * zy / 260;
            v = (char *)(Shim::vram + (399 - zy2) * PLANAR_WIDTH * 4);
            for (y = zly2; y <= zy2; y++)
            {
                memset(v, 255, PLANAR_WIDTH * 4);
                v += PLANAR_WIDTH * 4;
            }
            c = framez2;
            if (c > 32) c = 32;
            b = 32 - c;
            for (a = 0; a < 128 * 3; a++)
            {
                pal2[a] = static_cast<char>((pal1[a] * b + 30 * c) >> 5);
            }
            framez2++;
            demo_vsync();
            Common::setpalarea(pal2, 0, 128);
            demo_blit();
        }
        v = (char *)(Shim::vram + (194) * PLANAR_WIDTH * 4);
        Shim::outp(0x3c8, 0);
        Shim::outp(0x3c9, 0);
        Shim::outp(0x3c9, 0);
        Shim::outp(0x3c9, 0);
        v = (char *)(Shim::vram + (0) * PLANAR_WIDTH * 4);
        for (y = 0; y <= 399; y++)
        {
            memset(v, 0, PLANAR_WIDTH * 4);
            v += PLANAR_WIDTH * 4;
        }
    }

    void copper()
    {
        repeat++;

        Shim::outp(0x3c8, 0);

        for (int a = 0; a < 16 * 3; a++)
            Shim::outp(0x3c9, pal[a]);
    }

    void reset()
    {
        for (auto & col : backpal)
        {
            col = 16;
        }

        lightshift = 0;

        demomode[0] = demo_glz;
        demomode[1] = demo_glz;
        demomode[2] = demo_glz2;

        projxmul = 0;
        projymul = 0;
        projxadd = 0;
        projyadd = 0;
        projminz = 0;
        projminzshr = 0;

        wminx = 0;
        wminy = 0;
        wmaxx = 100;
        wmaxy = 100;

        count = 0;

        cntoff = 0;

        pointsoff = 0;

        CLEAR(bgpic);
        CLEAR(fcrow);
        CLEAR(fcrow2);

        CLEAR(pal1);
        CLEAR(pal2);

        CLEAR(points2);
        CLEAR(points2b);
        CLEAR(points3);

        CLEAR(edges2);

        CLEAR(polylist);
        CLEAR(matrix);

        CLEAR(pal);
        CLEAR(tmppal);

        repeat = {};
        frame = {};

        CLEAR(glenz_rows);

        CLEAR(newdata1);

        ndp0 = 0;
        ndp = 0;

        CLEAR(nep);
        CLEAR(nl);

        nec = NE_STRIDE;
        CLEAR(ne);

        yrow = 0;
        yrowad = 0;

        tmp_firstvx = {};
        tmp_color = {};
        siend = {};

        g_cf = 0;

        g_xadd = 0;
        g_yadd = 0;
        g_zadd = 0;

        CLEAR(g_m);
    }

    void main()
    {
        reset();

        int a{}, b{}, c{}, y{}, rx{}, ry{}, rz{}, r{}, g{}, zpos = 7500, y1{}, y2{}, ypos{}, yposa{};
        int ya{}, yy{}, boingm = 6, boingd = 7;
        int jello = 0, jelloa = 0;
        int xscale = 120, yscale = 120, zscale = 120, bscale = 0;
        int oxp = 0, oyp = 0, ozp = 0;
        int oxb = 0, oyb = 0, ozb = 0;
        char *ps{}, *pd{}, *pp{};

        if (!Shim::isDemoFirstPart())
        {
            while (!demo_wantstoquit() && Music::getPlusFlags() < -19)
            {
                demo_vsync();
                demo_blit();
            };
        }

        Music::setFrame(0);

        zoomer2();

        memset(Shim::vram, 0, 65535);
        demo_changemode(SCREEN_WIDTH, SCREEN_HEIGHT);
        init320x200C();

        for (a = 0; a < 100; a++)
        {
            fcrow[a] = (char *)(Data::fc + PaletteByteCount + 16 + a * SCREEN_WIDTH);
        }
        for (a = 0; a < 16; a++)
        {
            fcrow2[a] = (char *)(Data::fc + PaletteByteCount + 16 + a * SCREEN_WIDTH + 100 * SCREEN_WIDTH);
        }

        Shim::outp(0x3c8, 0);

        for (a = 0; a < static_cast<int>(PaletteByteCount); a++)
            Shim::outp(0x3c9, 0);

        Shim::outp(0x3c8, 0);

        for (a = 0; a < 16 * 3; a++)
            Shim::outp(0x3c9, Data::fc[a + 16]);

        yy = 0;
        ya = 0;

        while (!demo_wantstoquit())
        {
            ya++;
            yy += ya;
            if (yy > 48 * 16)
            {
                yy -= ya;
                ya = -ya * 2 / 3;
                if (ya > -4 && ya < 4) break;
            }
            y = yy / 16;
            y1 = 130 + y / 2;
            y2 = 130 + y * 3 / 2;
            if (y2 != y1) b = 25600 / (y2 - y1);
            pd = (char *)(Shim::vram + (y1 - 4) * SCREEN_WIDTH);

            for (ry = y1 - 4; ry < y1; ry++, pd += SCREEN_WIDTH)
            {
                if (ry > 199) continue;
                memset(pd, 0, SCREEN_WIDTH);
            }
            for (c = 0, ry = y1; ry < y2; ry++, pd += SCREEN_WIDTH, c += b)
            {
                if (ry > 199) continue;
                memcpy(pd, fcrow[c / 256], SCREEN_WIDTH);
            }
            for (c = 0; c < 16; c++, pd += SCREEN_WIDTH, ry++)
            {
                if (ry > 199) continue;
                if (c > 7)
                    memset(pd, 0, SCREEN_WIDTH);
                else
                    memcpy(pd, fcrow2[c], SCREEN_WIDTH);
            }
            demo_blit();
            demo_vsync();
        }

        while (!demo_wantstoquit() && Music::getFrame() < 300)
        {
            demo_vsync();
            demo_blit();
        }

        for (a = 0; a < 16; a++)
        {
            ps = (char *)(Data::fc + 0x10 + a * 3);
            pd = (char *)(backpal + a * 3);
            pd[0] = ps[0];
            pd[1] = ps[1];
            pd[2] = ps[2];
        }

        pp = tmppal;
        for (a = 0; a < 256; a++)
        {
            if (a < 16)
                b = a;
            else
            {
                b = a & 7;
            }
            r = backpal[b * 3 + 0];
            g = backpal[b * 3 + 1];
            b = backpal[b * 3 + 2];
            if ((a & 8) && a > 15)
            {
                r += 16;
                g += 16;
                b += 16;
            }
            if (r > 63) r = 63;
            if (g > 63) g = 63;
            if (b > 63) b = 63;
            *pp++ = static_cast<char>(r);
            *pp++ = static_cast<char>(g);
            *pp++ = static_cast<char>(b);
        }

        Shim::outp(0x3c8, 0);
        for (a = 0; a < static_cast<int>(PaletteByteCount); a++)
        {
            Shim::outp(0x3c9, tmppal[a]);
        }
        lightshift = 9;
        rx = ry = rz = 0;
        ypos = -9000;
        yposa = 0;
        demo_vsync();
        memcpy(bgpic, Shim::vram, SCREEN_SIZE);

        while (!demo_wantstoquit() && Music::getFrame() < 333)
        {
            demo_vsync();
            demo_blit();
        }

        memcpy(pal, backpal, 16 * 3);

        Shim::outp(0x3c7, 0);
        for (a = 0; a < 16 * 3; a++)
            pal[a] = Shim::inp(0x3c9);

        while (frame < 7000 && !demo_wantstoquit())
        {
            if (!Shim::isDemoFirstPart())
            {
                a = Music::getPlusFlags();
                if (a < 0 && a > -16) break;
            }

            repeat = demo_vsync();
            Shim::outp(0x3c8, 0);
            for (a = 0; a < 16 * 3; a++)
                Shim::outp(0x3c9, pal[a]);

            while (repeat--)
            {
                frame++;
                rx += 32;
                ry += 7;
                rx %= 3 * 3600;
                ry %= 3 * 3600;
                rz %= 3 * 3600;

                if (frame > 900)
                {
                    b = frame - 900;
                    a = frame - 900;
                    b = frame - 900;
                    if (b > 50) b = 50;
                    oxp = Common::Data::sin1024[(a * 3) & 1023] * b / 10;
                    oyp = Common::Data::sin1024[(a * 5) & 1023] * b / 10;
                    ozp = (Common::Data::sin1024[(a * 4) & 1023] / 2 + 128) * b / 16;
                    if (frame > 1800)
                    {
                        a = frame - 1800 + 64;
                        if (a > 1024) a = 1024;
                        oxb = (int32_t)(-Common::Data::sin1024[(a * 6) & 1023]) * (int32_t)a / 40L;
                        oyb = (int32_t)(-Common::Data::sin1024[(a * 7) & 1023]) * (int32_t)a / 40L;
                        ozb = (int32_t)(Common::Data::sin1024[(a * 8) & 1023] + 128) * (int32_t)a / 40L;
                    }
                    else
                    {
                        oxb = -Common::Data::sin1024[(a * 6) & 1023];
                        oyb = -Common::Data::sin1024[(a * 7) & 1023];
                        ozb = Common::Data::sin1024[(a * 8) & 1023] + 128;
                    }
                    b = 1800 - frame;
                    if (b < 0)
                    {
                        if (b < -99) b = -99;
                        oyp -= b * b / 2;
                    }
                }

                if (frame > 800)
                {
                    if (frame > 1220 + 789)
                    {
                        if (xscale > 0) xscale -= 1;
                        if (yscale > 0) yscale -= 1;
                        if (zscale > 0) zscale -= 1;
                        if (bscale > 0) bscale -= 1;
                    }
                    else if (frame > 1400 + 789)
                    {
                        if (bscale > 0) bscale -= 8;
                        if (bscale < 0) bscale = 0;
                    }
                    else
                    {
                        if (bscale < 180)
                            bscale += 2;
                        else
                            bscale = 180;
                    }
                    if (bscale > xscale) lightshift = 10;
                }
                else
                {
                    if (frame < 640 + 70)
                    {
                        yposa += 31;
                        ypos += yposa / 40;
                        if (ypos > -300)
                        {
                            ypos -= yposa / 40;
                            yposa = -yposa * boingm / boingd;
                            boingm += 2;
                            boingd++;
                        }
                        if (ypos > -900 && yposa > 0)
                        {
                            jello = (ypos + 900) * 5 / 3;
                            jelloa = 0;
                        }
                    }
                    else
                    {
                        if (ypos > -2800)
                            ypos -= 16;
                        else if (ypos < -2800)
                            ypos += 16;
                    }
                    yscale = xscale = 120 + jello / 30;
                    zscale = 120 - jello / 30;
                    a = jello;
                    jello += jelloa;
                    if ((a < 0 && jello > 0) || (a > 0 && jello < 0))
                    {
                        jelloa = jelloa * 5 / 6;
                    }
                    a = jello / 20;
                    jelloa -= a;
                }

                if (frame > 1280 + 789)
                {
                    b = 1280 + 789 + 64 - frame;
                    if (b < 0) b = 0;
                    for (a = 0; a < 16 * 3; a++)
                        pal[a] = static_cast<char>(backpal[a] * b / 64);
                }
                else if (frame > 700)
                {
                    if (frame < 765)
                    {
                        b = 764 - frame;
                        if (b < 0) b = 0;
                        for (a = 0; a < 16 * 3; a++)
                            pal[a] = static_cast<char>(backpal[a] * b / 64);
                    }
                    else if (frame < 790)
                    {
                        y = 150 + (frame - 765) * 2;
                        memset(bgpic + y * SCREEN_WIDTH, 0, 640);
                        memset(Shim::vram + y * SCREEN_WIDTH, 0, 640);
                        if (frame > 785)
                            for (a = 0; a < 16; a++)
                            {
                                r = g = b = 0;
                                if (a & 1)
                                {
                                    r += 10;
                                }
                                if (a & 2)
                                {
                                    r += 30;
                                }
                                if (a & 4)
                                {
                                    r += 20;
                                }
                                if (a & 8)
                                {
                                    r += 16;
                                    g += 16;
                                    b += 16;
                                }
                                if (r > 63) r = 63;
                                if (g > 63) g = 63;
                                if (b > 63) b = 63;
                                backpal[a * 3 + 0] = static_cast<char>(r);
                                backpal[a * 3 + 1] = static_cast<char>(g);
                                backpal[a * 3 + 2] = static_cast<char>(b);
                            }
                    }
                    else if (frame < 795)
                    {
                        memcpy(pal, backpal, 16 * 3);
                    }
                }
            }

            cglenzinit();

            if (xscale > 4)
            {
                demomode[0] = demomode[1];
                cmatrix_yxzC(static_cast<uint16_t>(rx), static_cast<uint16_t>(ry), static_cast<uint16_t>(rz), matrix);
                csetmatrix(matrix, 0, 0, 0);
                points2b[0] = 0;
                crotlist((int32_t *)points2b, (int32_t *)Data::points);
                matrix[0] = static_cast<short>(xscale * 64);
                matrix[1] = 0;
                matrix[2] = 0;
                matrix[3] = 0;
                matrix[4] = static_cast<short>(yscale * 64);
                matrix[5] = 0;
                matrix[6] = 0;
                matrix[7] = 0;
                matrix[8] = static_cast<short>(zscale * 64);
                csetmatrix(matrix, 0 + oxp, ypos + 1500 + oyp, zpos + ozp);
                points2[0] = 0;
                crotlist((int32_t *)points2, (int32_t *)points2b);
                if (frame < 800) cliplist((int32_t *)points2);
                points3[0] = 0;
                cprojlist((uint16_t *)points3, (int32_t *)points2);
                ceasypolylist(polylist, Data::epolys, (const int32_t *)points3);
                cglenzpolylist(polylist);
            }

            if (frame > 800 && bscale > 4)
            {
                demomode[0] = demomode[2];
                cmatrix_yxzC(static_cast<uint16_t>(3600 - rx / 3), static_cast<uint16_t>(3600 - ry / 3), static_cast<uint16_t>(3600 - rz / 3), matrix);
                csetmatrix(matrix, 0, 0, 0);
                points2b[0] = 0;
                crotlist((int32_t *)points2b, (int32_t *)Data::pointsb);
                matrix[0] = static_cast<short>(bscale * 64);
                matrix[1] = 0;
                matrix[2] = 0;
                matrix[3] = 0;
                matrix[4] = static_cast<short>(bscale * 64);
                matrix[5] = 0;
                matrix[6] = 0;
                matrix[7] = 0;
                matrix[8] = static_cast<short>(bscale * 64);
                csetmatrix(matrix, 0 + oxb, ypos + 1500 + oyb, zpos + ozb);
                points2[0] = 0;
                crotlist((int32_t *)points2, (int32_t *)points2b);
                points3[0] = 0;
                cprojlist((uint16_t *)points3, (int32_t *)points2);
                ceasypolylist(polylist, Data::epolysb, (int32_t *)points3);
                cglenzpolylist(polylist);
            }

            cglenzdone();
            demo_blit();
        }
    }
}
