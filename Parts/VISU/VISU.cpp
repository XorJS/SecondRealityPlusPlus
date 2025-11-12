#include "VISU.h"

#include "VISU_Data.h"

#include "Blob/Blob.h"

namespace Visu
{
    constexpr const short rowlen = SCREEN_WIDTH;

    constexpr const size_t MAXSIDES = 16;

    constexpr const size_t UNITSHR = 14;

    constexpr const size_t VF_UP = 1; /* direction in which a vertex is out of screen.*/
    constexpr const size_t VF_DOWN = 2;
    constexpr const size_t VF_LEFT = 4;
    constexpr const size_t VF_RIGHT = 8;
    constexpr const size_t VF_NEAR = 16; /* too close */
    constexpr const size_t VF_FAR = 32;

    /* flags for objects & faces (lower 8 bits in face flags are the side number) */
    constexpr const size_t F_DEFAULT = 0xf001; /* for objects only - all enabled, visible */
    constexpr const size_t F_VISIBLE = 0x0001; /* object visible */
    constexpr const size_t F_2SIDE = 0x0200;
    constexpr const size_t F_SHADE32 = 0x0C00;
    constexpr const size_t F_GOURAUD = 0x1000;

    using pred2 = int (*)(int16_t, int16_t);

    struct Poly
    {
        uint16_t sides, color, flags;
        const vlist * vxseg;
        struct
        {
            int16_t x, y;
        } v[MAXSIDES];
        const vlist * VX[MAXSIDES];
        uint16_t GR[MAXSIDES]; // 8.8 per-vertex color (Gouraud)
    };

    struct Stream
    {
        uint16_t * p;
    };

    namespace
    {
        unsigned char polyoversample = 0;      /* == projoversampleshr */
        unsigned char polyoversample16 = 16;   /* 16 - oversample */
        unsigned char polyoversamples = 1;     /* 1<<oversample */
        unsigned char polyoversample_mask = 0; /* polyoversamples-1 */

        int projclipx[2] = { 0, SCREEN_WIDTH - 1 };  // (xmin, xmax)
        int projclipy[2] = { 0, SCREEN_HEIGHT - 1 }; // (ymin, ymax)
        int projclipz[2] = { 256, 1000000000 };      // (zmin, zmax)

        int projmulx = 250;
        int projmuly = 220;
        int projaddx = 160;
        int projaddy = 100;

        unsigned short projaspect = 256; // aspect ratio(ratio = 256 * ypitch / xpitch)
        unsigned short projoversampleshr = 0;

        Poly poly1{}, poly2{};
        uint16_t polycury{};

        uint16_t polydrw[2048]{};

        object _objects[64]{};
        rmatrix _objectsMatrix[COUNT(_objects) * 2]{};
        vlist _vlist[4096]{};
        pvlist _pvlist[2048]{};
        unsigned char _memPool[160'000]{};

        object * objectsPool = _objects;
        rmatrix * objectsMatrixPool = _objectsMatrix;
        vlist * vlistPool = _vlist;
        pvlist * pvlistPool = _pvlist;
        unsigned char * memPool = _memPool;
    }

    char tmpname[64]{};

    char * scene0 = nullptr;
    char * scenem = nullptr;

    int city = 0;
    int xit = 0;

    s_scl scenelist[64]{};

    int scl = 0, sclp = 0;

    s_co co[MAXOBJ]{};
    int conum = 0;

    object camobject{};
    rmatrix cam{};

    int order[MAXOBJ]{}, ordernum{};

    unsigned char * sp = nullptr;

    s_cl cl[4]{};
    int clr = 0;
    int clw = 0;
    int firstframe = 1;
    int deadlock = 0;
    int coppercnt = 0;
    int syncframe = 0;
    int currframe = 0;
    int copperdelay = 16;
    int repeat = 0;
    int avgrepeat = 0;

    void resetMemoryPool()
    {
        objectsPool = _objects;
        objectsMatrixPool = _objectsMatrix;
        vlistPool = _vlist;
        pvlistPool = _pvlist;
        memPool = _memPool;
    }

    void reset()
    {
        resetMemoryPool();

        polyoversample = 0;      /* == projoversampleshr */
        polyoversample16 = 16;   /* 16 - oversample */
        polyoversamples = 1;     /* 1<<oversample */
        polyoversample_mask = 0; /* polyoversamples-1 */

        projclipz[0] = 256;
        projclipz[1] = 1000000000L;

        projclipx[0] = 0;
        projclipx[1] = (SCREEN_WIDTH - 1);
        projclipy[0] = 0;
        projclipy[1] = (SCREEN_HEIGHT - 1);

        projmulx = 250;
        projmuly = 220;
        projaddx = 160;
        projaddy = 100;

        projaspect = 256; // aspect ratio(ratio = 256 * ypitch / xpitch)
        projoversampleshr = 0;

        poly1 = {};
        poly2 = {};
        polycury = {};

        CLEAR(polydrw);
        CLEAR(tmpname);

        scene0 = nullptr;
        scenem = nullptr;

        city = 0;
        xit = 0;

        CLEAR(scenelist);

        scl = 0;
        sclp = 0;

        CLEAR(co);

        conum = 0;

        camobject = {};
        cam = {};

        CLEAR(order);
        ordernum = {};

        sp = nullptr;

        CLEAR(cl);

        clr = 0;
        clw = 0;
        firstframe = 1;
        deadlock = 0;
        coppercnt = 0;
        syncframe = 0;
        currframe = 0;
        copperdelay = 16;
        repeat = 0;
        avgrepeat = 0;
    }

    object * getNewObject()
    {
        return objectsPool++;
    }

    rmatrix * getNewRMatrix()
    {
        return objectsMatrixPool++;
    }

    object * loadobject(char * fname)
    {
        int error;
        int a, b = 0;
        object * o;
        int32_t l;
        char *d, *d0;

        d = readfile(fname);
        o = objectsPool++;

        o->flags = F_DEFAULT;
        o->r = objectsMatrixPool++;
        o->r0 = objectsMatrixPool++;

        memset(o->r, 0, sizeof(rmatrix));
        memset(o->r0, 0, sizeof(rmatrix));

        o->vnum = 0;
        o->nnum = 0;
        o->v0 = nullptr;
        o->n0 = nullptr;
        o->v = nullptr;
        o->n = nullptr;
        o->pv = nullptr;
        o->plnum = 1;

        for (;;)
        {
            error = 0;
            d0 = d;
            d += 8;

            if (!memcmp(d0, "END ", 4))
                break;
            else if (!memcmp(d0, "VERS", 4))
            {
                a = *(((short *)d));
                d += 2;

                if (a != 0x100)
                {
                    //"Version not 1.00";
                }
            }
            else if (!memcmp(d0, "NAME", 4))
            {
                o->name = (char *)d;
            }
            else if (!memcmp(d0, "VERT", 4))
            {
                o->vnum = *(((short *)d));
                d += 2;
                d += 2;

                o->v0 = (vlist *)d;
                o->v = vlistPool;
                o->pv = pvlistPool;
                vlistPool += o->vnum;
                pvlistPool += o->vnum;
            }
            else if (!memcmp(d0, "NORM", 4))
            {
                o->nnum = *(((short *)d));
                d += 2;
                o->nnum1 = *(((short *)d));
                d += 2;
                o->n0 = (Visu::nlist *)d;
                o->n = (Visu::nlist *)vlistPool;
                vlistPool += o->nnum;
            }
            else if (!memcmp(d0, "POLY", 4))
            {
                o->pd = (polydata *)d;
            }
            else if (!memcmp(d0, "ORD", 3))
            {
                a = d0[3];
                if (a == '0')
                    b = 0;
                else if (a == 'E')
                    b = o->plnum++;
                else
                    error = 1;
                if (!error)
                {
                    o->pl[b] = (polylist *)d;
                }
            }
            else
                error = 1;

            l = *(int32_t *)(d0 + 4);
            d = d0 + l + 8;
        }

        return (o);
    }

    static inline int32_t fx_dot3_shift14(int32_t a0, int32_t b0, int32_t a1, int32_t b1, int32_t a2, int32_t b2)
    {
        /* Accumulate full 64-bit then arithmetic shift by UNITSHR (matches SHRD). */
        int64_t acc = (int64_t)a0 * (int64_t)b0;
        acc += (int64_t)a1 * (int64_t)b1;
        acc += (int64_t)a2 * (int64_t)b2;
        return (int32_t)(acc >> UNITSHR);
    }

    static inline int32_t m32(const rmatrix * m, int idx)
    {
        return (int32_t)m->m[idx];
    }

    static inline int32_t m16(const rmatrix * m, int idx)
    {
        return (int32_t)(int16_t)(m->m[idx] & 0xFFFF);
    }

    /* Overwrite right (m2 = m1 * m2), mirroring mulmatrices2 */
    static void mul_matrices_overwrite_right(const int32_t * m1, int32_t * m2)
    {
        int32_t r[9];
        r[0] = fx_dot3_shift14(m1[0], m2[0], m1[1], m2[3], m1[2], m2[6]);
        r[1] = fx_dot3_shift14(m1[0], m2[1], m1[1], m2[4], m1[2], m2[7]);
        r[2] = fx_dot3_shift14(m1[0], m2[2], m1[1], m2[5], m1[2], m2[8]);

        r[3] = fx_dot3_shift14(m1[3], m2[0], m1[4], m2[3], m1[5], m2[6]);
        r[4] = fx_dot3_shift14(m1[3], m2[1], m1[4], m2[4], m1[5], m2[7]);
        r[5] = fx_dot3_shift14(m1[3], m2[2], m1[4], m2[5], m1[5], m2[8]);

        r[6] = fx_dot3_shift14(m1[6], m2[0], m1[7], m2[3], m1[8], m2[6]);
        r[7] = fx_dot3_shift14(m1[6], m2[1], m1[7], m2[4], m1[8], m2[7]);
        r[8] = fx_dot3_shift14(m1[6], m2[2], m1[7], m2[5], m1[8], m2[8]);

        for (int i = 0; i < 9; i++)
            m2[i] = r[i];
    }

    /* Rotate a single 3-vector by 3x3 */
    static void rotate_single_vec(const rmatrix * rm, const int32_t src_xyz[3], int32_t dst_xyz[3])
    {
        const int32_t * M = (const int32_t *)rm->m;
        int32_t x = (int32_t)src_xyz[0];
        int32_t y = (int32_t)src_xyz[1];
        int32_t z = (int32_t)src_xyz[2];

        dst_xyz[0] = fx_dot3_shift14(x, M[0], y, M[1], z, M[2]);
        dst_xyz[1] = fx_dot3_shift14(x, M[3], y, M[4], z, M[5]);
        dst_xyz[2] = fx_dot3_shift14(x, M[6], y, M[7], z, M[8]);
    }

    /* dest = (apply * dest) for rotation; then rotate dest->pos by apply->m; then translate by apply->pos */
    void calc_applyrmatrix(rmatrix * dest, rmatrix * apply)
    {
        mul_matrices_overwrite_right((const int32_t *)apply->m, (int32_t *)dest->m);

        int32_t tmp[3] = { dest->x, dest->y, dest->z };
        rotate_single_vec(apply, tmp, tmp);
        dest->x = tmp[0];
        dest->y = tmp[1];
        dest->z = tmp[2];

        dest->x += apply->x;
        dest->y += apply->y;
        dest->z += apply->z;
    }

    void calc_rotate(int count, vlist * dest, vlist * source, rmatrix * matrix)
    {
        if (count <= 0) return;
        for (int i = 0; i < count; ++i)
        {
            int32_t sx = (int32_t)source[i].x;
            int32_t sy = (int32_t)source[i].y;
            int32_t sz = (int32_t)source[i].z;

            int32_t x = fx_dot3_shift14(sx, m16(matrix, 0), sy, m16(matrix, 1), sz, m16(matrix, 2)) + matrix->x;
            int32_t y = fx_dot3_shift14(sx, m16(matrix, 3), sy, m16(matrix, 4), sz, m16(matrix, 5)) + matrix->y;
            int32_t z = fx_dot3_shift14(sx, m16(matrix, 6), sy, m16(matrix, 7), sz, m16(matrix, 8)) + matrix->z;

            dest[i].x = x;
            dest[i].y = y;
            dest[i].z = z;
            dest[i].normal = source[i].normal;
        }
    }

    /* Rotate ONLY normals (short) with 16-bit reads/writes and 16-bit matrix elements. */
    void calc_nrotate(int count, nlist * dest, nlist * source, rmatrix * matrix)
    {
        if (count <= 0) return;
        for (int i = 0; i < count; ++i)
        {
            int32_t sx = (int32_t)(int16_t)source[i].x;
            int32_t sy = (int32_t)(int16_t)source[i].y;
            int32_t sz = (int32_t)(int16_t)source[i].z;

            int32_t x = fx_dot3_shift14(sx, m16(matrix, 0), sy, m16(matrix, 1), sz, m16(matrix, 2));
            int32_t y = fx_dot3_shift14(sx, m16(matrix, 3), sy, m16(matrix, 4), sz, m16(matrix, 5));
            int32_t z = fx_dot3_shift14(sx, m16(matrix, 6), sy, m16(matrix, 7), sz, m16(matrix, 8));

            dest[i].x = (short)x;
            dest[i].y = (short)y;
            dest[i].z = (short)z;
        }
    }

    /* Rotate single vertex and return resulting Z */
    int32_t calc_singlez(int vertexnum, vlist * vertexlist, rmatrix * matrix)
    {
        vlist * v = &vertexlist[vertexnum];
        int32_t z = fx_dot3_shift14((int32_t)v->x, m32(matrix, 6), (int32_t)v->y, m32(matrix, 7), (int32_t)v->z, m32(matrix, 8));
        z += matrix->z;
        return (int32_t)z;
    }

    int calc_project(int count, pvlist * dest, vlist * source)
    {
        int vf_all = 0xFFFF;
        for (int i = 0; i < count; ++i)
        {
            int32_t X = (int32_t)source[i].x;
            int32_t Y = (int32_t)source[i].y;
            int32_t Z = (int32_t)source[i].z;

            int16_t vf = 0;
            /* Z clip */
            if (Z < projclipz[0])
            {
                vf |= 16;
                Z = (int32_t)projclipz[0];
            }
            else if (Z > projclipz[1])
            {
                vf |= 32; /* use Z as-is */
            }

            /* Y = projmuly * Y / Z + projaddy */
            int32_t y = (int32_t)((int64_t)projmuly * (int64_t)Y / (int32_t)Z) + (int32_t)projaddy;
            if (y > projclipy[1]) vf |= 2;
            if (y < projclipy[0]) vf |= 1;
            dest[i].y = (short)y;

            int32_t x = (int32_t)((int64_t)projmulx * (int64_t)X / (int32_t)Z) + (int32_t)projaddx;
            if (x > projclipx[1]) vf |= 8;
            if (x < projclipx[0]) vf |= 4;
            dest[i].x = (short)x;

            dest[i].vf = vf;
            vf_all &= vf;
        }
        return vf_all;
    }

    void cameraangle(angle a)
    {
        /* halfw = right_edge - center_x */
        int32_t halfw = (int32_t)(projclipx[1] - projaddx);

        int32_t bx = ((int32_t)a) >> 1; /* half of angle */
        if (bx < 8 * 64) bx = 8 * 64;
        if (bx >= 16384) bx = 16383;
        bx = (bx >> (6 - 1)) & ~1; /* even word index */

        uint16_t av = Data::avistan[(uint32_t)bx >> 1]; /* word table */
        int64_t p = (int64_t)halfw * (int64_t)av;
        projmulx = (int32_t)(p >> 8);

        int64_t p2 = (int64_t)projmulx * (int64_t)projaspect;
        projmuly = (int32_t)(p2 >> 8);
    }

    void window(int32_t x1, int32_t x2, int32_t y1, int32_t y2, int32_t z1, int32_t z2)
    {
        projclipx[0] = x1;
        projclipx[1] = x2;
        projaddx = (x1 + x2) >> 1;
        projclipy[0] = y1;
        projclipy[1] = y2;
        projaddy = (y1 + y2) >> 1;
        projclipz[0] = z1;
        projclipz[1] = z2;
    }

    void clear255()
    {
        uint8_t * dst = Shim::vram;

        const int inner_bytes = (SCREEN_WIDTH / 4 / 4) * 4;
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            for (int i = 0; i < inner_bytes; ++i)
            {
                dst[i] = 0xFF;
            }
            dst += inner_bytes;
        }
    }

    void clearbg(char * bg)
    {
        if (!bg) return;

        memcpy(Shim::vram, (const void *)bg, (size_t)(SCREEN_SIZE));
    }

    void init()
    {
        /* proj clip / center / scale */
        projclipz[0] = 256;
        projclipz[1] = 1000000000L;

        projclipx[0] = 0;
        projclipx[1] = (SCREEN_WIDTH - 1);
        projclipy[0] = 0;
        projclipy[1] = 199;

        projmulx = 250;
        projmuly = 220;
        projaddx = 160;
        projaddy = 100;

        projoversampleshr = 0;
        projaspect = 225;
    }

    static inline int culled_by_backface(const nlist * n, const vlist * v0)
    {
        int64_t dot = (int64_t)n->x * v0->x + (int64_t)n->y * v0->y + (int64_t)n->z * v0->z;

        return dot >= 0; // invisible when dot >= 0
    }

    static inline uint16_t normallight_u8(const nlist * n)
    {
        static const int16_t L[3] = { 12118, 10603, 3030 };

        int64_t acc = (int64_t)n->x * L[0] + (int64_t)n->y * L[1] + (int64_t)n->z * L[2];

        int v = (int)(acc >> 21); // == take hiword, then >>5 ? acc>>21

        v += 128;
        if (v < 0) v = 0;
        if (v > 255) v = 255;

        return (uint16_t)v;
    }

    static inline uint16_t calclight_quant(uint16_t flags, const nlist * n)
    {
        uint16_t f = flags & F_SHADE32;

        if (!f) return 0;

        uint16_t nl = normallight_u8(n);
        unsigned dx = f >> 10; // 1,2,3
        unsigned sh = 6 - dx;  // 5,4,3

        uint16_t q = (uint16_t)(nl >> sh);
        if (q < 1) q = 1;
        if (q > 30) q = 30;

        return q;
    }

    static inline void fast_stosb(uint8_t * dst, int count, uint8_t val)
    {
        if (count > 0 && dst) memset(dst, val, (size_t)count);
    }

    static inline int x_hiword_u(int32_t q)
    {
        return (int)((uint32_t)q >> 16);
    }

    static void drawfill_nrm(uint16_t * s)
    {
        uint8_t color = (uint8_t)(*s++);
        uint16_t startY = *s++;
        uint8_t * di = Shim::vram + Data::rows[startY];

        int32_t lx = 0, rx = 0, la = 0, ra = 0; // 16.16 accumulators & per-row adds

        for (;;)
        {
            int16_t tag = (int16_t)(*s++);

            if (tag < 0) return; // left tag

            if (tag)
            {
                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;

                t.w[0] = *s++;
                t.w[1] = *s++;
                lx = (int32_t)t.u; // Xstart (16.16 + 0x8000 bias already baked)

                t.w[0] = *s++;
                t.w[1] = *s++;
                la = (int32_t)t.u; // Xadd per row
            }

            tag = (int16_t)(*s++);

            if (tag < 0) return; // right tag

            if (tag)
            {
                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;

                t.w[0] = *s++;
                t.w[1] = *s++;
                rx = (int32_t)t.u; // Xstart

                t.w[0] = *s++;
                t.w[1] = *s++;
                ra = (int32_t)t.u; // Xadd
            }

            int cnt = *s++;
            if (cnt <= 0) return;

            do
            {
                // PRE-STEP edges (ASM does add before sampling HIWORD)
                lx += la;
                rx += ra;

                int xL = x_hiword_u(lx);
                int xR = x_hiword_u(rx);

                if (xL == xR)
                {
                    di += rowlen;
                    continue;
                }
                if (xR < xL)
                {
                    int t = xR;
                    xR = xL;
                    xL = t;
                }

                // safety clamp to framebuffer; ASM assumes clipped already
                if (xL < 0) xL = 0;
                if (xR > rowlen) xR = rowlen;

                int span = xR - xL; // right-exclusive [xL, xR)
                if (span > 0) fast_stosb(di + xL, span, color);

                di += rowlen;
            } while (--cnt);
        }
    }

    // Gouraud fill (8.8 edge colors, top byte is the pixel)
    static void drawfill_grd(uint16_t * s)
    {
        (void)*s++; // stream color word
        uint16_t startY = *s++;
        uint8_t * di = Shim::vram + Data::rows[startY];

        int32_t lx = 0, rx = 0, la = 0, ra = 0; // 16.16 x accumulators and per-row adds
        uint16_t lc = 0, rc = 0;                // 8.8 edge colors
        int16_t lca = 0, rca = 0;               // 8.8 per-row color adds

        for (;;)
        {
            int16_t tag = (int16_t)(*s++);

            if (tag < 0) return; // left tag

            if (tag)
            {
                lc = *s++;
                lca = (int16_t)*s++;
                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;
                t.w[0] = *s++;
                t.w[1] = *s++;
                lx = (int32_t)t.u; // Xstart
                t.w[0] = *s++;
                t.w[1] = *s++;
                la = (int32_t)t.u; // Xadd
            }

            tag = (int16_t)(*s++);

            if (tag < 0) return; // right tag

            if (tag)
            {
                rc = *s++;
                rca = (int16_t)*s++;
                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;

                t.w[0] = *s++;
                t.w[1] = *s++;
                rx = (int32_t)t.u; // Xstart

                t.w[0] = *s++;
                t.w[1] = *s++;
                ra = (int32_t)t.u; // Xadd
            }

            int cnt = *s++;

            if (cnt <= 0) return;

            do
            {
                lx += la;
                rx += ra;
                lc = (uint16_t)(lc + lca);
                rc = (uint16_t)(rc + rca);

                int xL = (int)((uint32_t)lx >> 16);
                int xR = (int)((uint32_t)rx >> 16);

                // fetch top bytes of the 8.8 edge colors
                int cL = (lc >> 8) & 0xFF;
                int cR = (rc >> 8) & 0xFF;

                if (xL == xR)
                {
                    di += rowlen;
                    continue;
                }

                if (xR < xL)
                {
                    int t = xR;
                    xR = xL;
                    xL = t;
                    int tc = cR;
                    cR = cL;
                    cL = tc;
                }

                if (xL < 0) xL = 0;
                if (xR > rowlen) xR = rowlen;

                if (int span = xR - xL; span > 0) // right-exclusive
                {
                    // Bresenham-style integer ramp
                    int diff = cR - cL;
                    int step = diff / span;
                    int rem = diff % span;
                    int acc = 0;
                    int sgn = (rem >= 0) ? 1 : -1;
                    int rabs = (rem >= 0) ? rem : -rem;

                    int c = cL;
                    uint8_t * p = di + xL;
                    for (int i = 0; i < span; ++i)
                    {
                        p[i] = (uint8_t)c;
                        acc += rabs;
                        if (acc >= span)
                        {
                            c += step + sgn;
                            acc -= span;
                        }
                        else
                        {
                            c += step;
                        }
                    }
                }

                di += rowlen;

            } while (--cnt);
        }
    }

    static void vid_drawfill(uint16_t * stream)
    {
        if (uint16_t type = *stream++; type == 0)
            drawfill_nrm(stream);
        else if (type == 1)
            drawfill_grd(stream);
    }

    static inline void edge_setup_nrm(int16_t x0, int16_t x1, int height, uint32_t * Xstart, int32_t * Xadd)
    {
        int sh = 16 - polyoversample;

        // This gives us 16.16 fixed-point with 0.5 pixel center offset
        int32_t xs = ((int32_t)x0 << sh);
        xs = (xs & 0xFFFF0000) | 0x8000;

        int32_t xe = ((int32_t)x1 << sh);
        xe = (xe & 0xFFFF0000) | 0x8000;

        int32_t pre = 0;
        if (height > 0)
        {
            pre = (int32_t)(((int64_t)(xe - xs)) / height);
        }

        uint8_t sub = (uint8_t)(polycury & polyoversample_mask);
        if (sub)
        {
            xs -= (int32_t)((int64_t)pre * sub);
        }

        int32_t xa = pre << polyoversample;

        *Xstart = (uint32_t)xs;
        *Xadd = xa;
    }

    static inline void edge_setup_grd(int16_t x0, int16_t x1, int height, uint16_t c0, uint16_t c1, uint16_t * Cstart, int16_t * Cadd, uint32_t * Xstart,
                                      int32_t * Xadd)
    {
        // Color interpolation (straightforward)
        *Cstart = c0;
        int16_t ca = 0;
        if (height > 0)
        {
            ca = (int16_t)((int32_t)(c1 - c0) / height);
        }
        *Cadd = ca;

        // Color pre-step
        uint8_t sub = (uint8_t)(polycury & polyoversample_mask);
        if (sub)
        {
            *Cstart = (uint16_t)((int32_t)(*Cstart) - (int32_t)ca * (int32_t)sub);
        }

        // X interpolation
        edge_setup_nrm(x0, x1, height, Xstart, Xadd);
    }

    static inline void sw_header(Stream * s, uint16_t color, uint16_t startY)
    {
        *s->p++ = color;
        *s->p++ = startY;
    }

    static inline void sw_left(Stream * s, int present, uint32_t xs, int32_t xa)
    {
        if (*s->p++ = (uint16_t)(present ? 1 : 0); present)
        {
            union
            {
                uint32_t u;
                uint16_t w[2];
            } t;

            t.u = xs;
            *s->p++ = t.w[0];
            *s->p++ = t.w[1];

            t.u = (uint32_t)xa;
            *s->p++ = t.w[0];
            *s->p++ = t.w[1];
        }
    }

    static inline void sw_right(Stream * s, int present, uint32_t xs, int32_t xa)
    {
        if (*s->p++ = (uint16_t)(present ? 1 : 0); present)
        {
            union
            {
                uint32_t u;
                uint16_t w[2];
            } t;

            t.u = xs;
            *s->p++ = t.w[0];
            *s->p++ = t.w[1];

            t.u = (uint32_t)xa;
            *s->p++ = t.w[0];
            *s->p++ = t.w[1];
        }
    }

    static inline void sw_count(Stream * s, uint16_t rows_os)
    {
        *s->p++ = rows_os;
    }

    static inline void sw_end(Stream * s)
    {
        *s->p++ = 0xFFFF;
    }

    static void poly_nrm_build(const Poly * P, uint16_t * out)
    {
        // find top and bottom scanline
        int top = P->v[0].y, bottom = top, topi = 0;
        for (int i = 1; i < P->sides; ++i)
        {
            int y = P->v[i].y;
            if (y < top)
            {
                top = y;
                topi = i;
            }
            if (y > bottom) bottom = y;
        }
        polycury = (uint16_t)top;

        Stream S = { .p = out };

        sw_header(&S, P->color, (uint16_t)((int)top >> polyoversample));

        if (top == bottom)
        {
            sw_end(&S);
            return;
        }

        int li = topi, ri = topi;
        uint32_t lxs = 0, rxs = 0;
        int32_t lxa = 0, rxa = 0;
        uint16_t lh = 0, rh = 0;

        for (;;)
        {
            // LEFT EDGE: start a new segment whenever the current vertex y matches the sweep
            if (P->v[li].y == (int16_t)polycury)
            {
                // walk backwards, skipping horizontal edges
                int prev = (li == 0) ? (P->sides - 1) : (li - 1);
                int dy = (int)P->v[prev].y - (int)P->v[li].y;

                while (dy == 0)
                { // skip flat segment
                    li = prev;
                    prev = (li == 0) ? (P->sides - 1) : (li - 1);
                    dy = (int)P->v[prev].y - (int)P->v[li].y;
                    if (li == topi) break; // safety: closed loop
                }

                if (dy <= 0)
                {
                    sw_end(&S);
                    return;
                } // degenerate (shouldn't happen for CCW polys)

                lh = (uint16_t)dy;
                edge_setup_nrm(P->v[li].x, P->v[prev].x, dy, &lxs, &lxa);
                li = prev;
                sw_left(&S, 1, lxs, lxa);
            }
            else
            {
                sw_left(&S, 0, 0, 0);
            }

            // RIGHT EDGE
            if (P->v[ri].y == (int16_t)polycury)
            {
                // walk forwards, skipping horizontal edges
                int next = (ri + 1 == P->sides) ? 0 : (ri + 1);
                int dy = (int)P->v[next].y - (int)P->v[ri].y;

                while (dy == 0)
                { // skip flat segment
                    ri = next;
                    next = (ri + 1 == P->sides) ? 0 : (ri + 1);
                    dy = (int)P->v[next].y - (int)P->v[ri].y;
                    if (ri == topi) break; // safety
                }

                if (dy <= 0)
                {
                    sw_end(&S);
                    return;
                }

                rh = (uint16_t)dy;
                edge_setup_nrm(P->v[ri].x, P->v[next].x, dy, &rxs, &rxa);

                ri = next;
                sw_right(&S, 1, rxs, rxa);
            }
            else
            {
                sw_right(&S, 0, 0, 0);
            }

            // emit the span count for the shorter of the two active edges
            uint16_t shorter = (lh < rh) ? lh : rh;
            uint16_t rows_os = (uint16_t)((shorter >> polyoversample));
            sw_count(&S, rows_os);

            lh -= shorter;
            rh -= shorter;
            polycury += shorter;

            // stop once we marched past bottom
            if ((int)polycury >= bottom) break;
        }
        sw_end(&S);
    }

    static void poly_grd_build(const Poly * P, uint16_t * out)
    {
        int top = P->v[0].y, bottom = top, topi = 0;

        for (int i = 1; i < P->sides; ++i)
        {
            int y = P->v[i].y;
            if (y < top)
            {
                top = y;
                topi = i;
            }
            if (y > bottom) bottom = y;
        }
        polycury = (uint16_t)top;

        Stream S = { .p = out };

        sw_header(&S, P->color, (uint16_t)((int)top >> polyoversample));

        if (top == bottom)
        {
            sw_end(&S);
            return;
        }

        int li = topi, ri = topi;
        uint32_t lxs = 0, rxs = 0;
        int32_t lxa = 0, rxa = 0;
        uint16_t lc = 0, rc = 0;
        int16_t lca = 0, rca = 0;
        uint16_t lh = 0, rh = 0;

        for (;;)
        {
            // LEFT EDGE (skip horizontals)
            if (P->v[li].y == (int16_t)polycury)
            {
                int prev = (li == 0) ? (P->sides - 1) : (li - 1);
                int dy = (int)P->v[prev].y - (int)P->v[li].y;

                while (dy == 0)
                {
                    li = prev;
                    prev = (li == 0) ? (P->sides - 1) : (li - 1);
                    dy = (int)P->v[prev].y - (int)P->v[li].y;
                    if (li == topi) break;
                }

                if (dy <= 0)
                {
                    sw_end(&S);
                    return;
                }

                lh = (uint16_t)dy;
                edge_setup_grd(P->v[li].x, P->v[prev].x, dy, P->GR[li], P->GR[prev], &lc, &lca, &lxs, &lxa);

                *S.p++ = 1;
                *S.p++ = lc;
                *S.p++ = (uint16_t)lca;

                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;

                t.u = lxs;
                *S.p++ = t.w[0];
                *S.p++ = t.w[1];

                t.u = (uint32_t)lxa;
                *S.p++ = t.w[0];
                *S.p++ = t.w[1];
                li = prev;
            }
            else
            {
                *S.p++ = 0;
            }

            // RIGHT EDGE (skip horizontals)
            if (P->v[ri].y == (int16_t)polycury)
            {
                int next = (ri + 1 == P->sides) ? 0 : (ri + 1);
                int dy = (int)P->v[next].y - (int)P->v[ri].y;

                while (dy == 0)
                {
                    ri = next;
                    next = (ri + 1 == P->sides) ? 0 : (ri + 1);
                    dy = (int)P->v[next].y - (int)P->v[ri].y;
                    if (ri == topi) break;
                }

                if (dy <= 0)
                {
                    sw_end(&S);
                    return;
                }

                rh = (uint16_t)dy;
                edge_setup_grd(P->v[ri].x, P->v[next].x, dy, P->GR[ri], P->GR[next], &rc, &rca, &rxs, &rxa);

                *S.p++ = 1;
                *S.p++ = rc;
                *S.p++ = (uint16_t)rca;

                union
                {
                    uint32_t u;
                    uint16_t w[2];
                } t;

                t.u = rxs;
                *S.p++ = t.w[0];
                *S.p++ = t.w[1];

                t.u = (uint32_t)rxa;
                *S.p++ = t.w[0];
                *S.p++ = t.w[1];
                ri = next;
            }
            else
            {
                *S.p++ = 0;
            }

            uint16_t shorter = (lh < rh) ? lh : rh;
            uint16_t rows_os = (uint16_t)((shorter >> polyoversample));
            sw_count(&S, rows_os);

            lh -= shorter;
            rh -= shorter;
            polycury += shorter;

            if ((int)polycury >= bottom) break;
        }
        sw_end(&S);
    }

    static inline void zclip_intersect_xy(const vlist * farV, const vlist * nearV, int clipZ, int16_t * outX, int16_t * outY)
    {
        int64_t dz = (int64_t)farV->z - (int64_t)nearV->z;
        if (dz == 0) dz = 1; // avoid division by zero

        int64_t m = (int64_t)farV->z - (int64_t)clipZ;

        // Use higher precision for 3D interpolation
        // Xi = farV->x + (nearV->x - farV->x) * m / dz
        int64_t dx = (int64_t)nearV->x - (int64_t)farV->x;
        int64_t dy = (int64_t)nearV->y - (int64_t)farV->y;

        // Scale up by 16 bits for precision, then divide
        int64_t Xi = (int64_t)farV->x + ((dx * m) / dz);
        int64_t Yi = (int64_t)farV->y + ((dy * m) / dz);

        // Project with proper precision
        int32_t x = (int32_t)(((int64_t)projmulx * Xi) / clipZ) + projaddx;
        int32_t y = (int32_t)(((int64_t)projmuly * Yi) / clipZ) + projaddy;

        *outX = (int16_t)x;
        *outY = (int16_t)y;
    }

    // Return the OR of 2D out-bits that belong to *inserted* vertices only.
    static uint8_t clip_near_reproject(const Poly * in, Poly * out)
    {
        int di = 0;
        int clipZ = projclipz[0];
        uint8_t new_or = 0;

        for (int i = 0; i < in->sides; i++)
        {
            int j = (i + 1 == in->sides) ? 0 : (i + 1);
            const vlist * Vi = in->VX[i];
            const vlist * Vj = in->VX[j];
            int zi = Vi->z, zj = Vj->z;
            int Ii = (zi >= clipZ), Ij = (zj >= clipZ);

            if (Ii)
            { // pass-through vertex
                out->v[di] = in->v[i];
                out->GR[di] = in->GR[i];
                out->VX[di] = in->VX[i];
                di++;
            }

            if (Ii ^ Ij)
            {
                int16_t Xc, Yc;
                if (zi >= zj)
                    zclip_intersect_xy(Vi, Vj, clipZ, &Xc, &Yc);
                else
                    zclip_intersect_xy(Vj, Vi, clipZ, &Xc, &Yc);

                // Interpolate Gouraud color for inserted vertex
                uint16_t gI = in->GR[i];
                if (in->flags & F_GOURAUD)
                {
                    int64_t dz = (int64_t)Vi->z - (int64_t)Vj->z;
                    if (dz != 0)
                    {
                        int64_t m = (int64_t)Vi->z - (int64_t)clipZ;
                        int32_t dg = (int32_t)in->GR[j] - (int32_t)in->GR[i];
                        gI = (uint16_t)(in->GR[i] + (int32_t)((dg * m) / dz));
                    }
                }

                out->v[di].x = Xc;
                out->v[di].y = Yc;
                out->GR[di] = gI; // Use interpolated color instead of 0
                out->VX[di] = in->VX[i];

                // compute its *2D* out-bits (UP/DOWN/LEFT/RIGHT only)
                if (Yc < projclipy[0]) new_or |= VF_UP;
                if (Yc > projclipy[1]) new_or |= VF_DOWN;
                if (Xc < projclipx[0]) new_or |= VF_LEFT;
                if (Xc > projclipx[1]) new_or |= VF_RIGHT;

                di++;
            }
        }

        out->sides = (uint16_t)di;
        out->vxseg = in->vxseg;
        out->flags = in->flags;
        out->color = in->color;

        return new_or;
    }

    static inline int out_up(int16_t x, int16_t y)
    {
        (void)x;
        return y < projclipy[0];
    }

    static inline int out_down(int16_t x, int16_t y)
    {
        (void)x;
        return y > projclipy[1];
    }

    static inline int out_left(int16_t x, int16_t y)
    {
        (void)y;
        return x < projclipx[0];
    }

    static inline int out_right(int16_t x, int16_t y)
    {
        (void)y;
        return x > projclipx[1];
    }

    static void side_clip(const Poly * in, Poly * out, pred2 isOut, int horiz, int clipv)
    {
        int di = 0;
        for (int i = 0; i < in->sides; i++)
        {
            int j = (i + 1 == in->sides) ? 0 : (i + 1);
            int16_t xi = in->v[i].x, yi = in->v[i].y;
            int16_t xj = in->v[j].x, yj = in->v[j].y;
            int oi = isOut(xi, yi), oj = isOut(xj, yj);

            if (!oi)
            {
                out->v[di] = in->v[i];
                out->GR[di] = in->GR[i];
                out->VX[di] = in->VX[i];
                di++;
            }

            if (oi ^ oj)
            {
                // Fixed-point interpolation (14-bit fraction)
                int16_t Xc, Yc;

                if (horiz)
                {
                    // Clipping horizontal (Y = constant)
                    int32_t dy = (int32_t)yj - yi;
                    int32_t dx = (int32_t)xj - xi;
                    int32_t dist = (int32_t)clipv - yi;

                    if (dy != 0)
                    {
                        // slope = (dist << 14) / dy
                        int64_t slope = ((int64_t)dist << 14) / dy;

                        // Xc = xi + (dx * slope) >> 14
                        Xc = (int16_t)(xi + (int32_t)((dx * slope) >> 14));
                    }
                    else
                    {
                        Xc = xi;
                    }
                    Yc = (int16_t)clipv;
                }
                else
                {
                    // Clipping vertical (X = constant)
                    int32_t dx = (int32_t)xj - xi;
                    int32_t dy = (int32_t)yj - yi;
                    int32_t dist = (int32_t)clipv - xi;

                    if (dx != 0)
                    {
                        // slope = (dist << 14) / dx
                        int64_t slope = ((int64_t)dist << 14) / dx;

                        // Yc = yi + (dy * slope) >> 14
                        Yc = (int16_t)(yi + (int32_t)((dy * slope) >> 14));
                    }
                    else
                    {
                        Yc = yi;
                    }
                    Xc = (int16_t)clipv;
                }

                // Interpolate Gouraud color with same precision
                uint16_t g0 = in->GR[i], g1 = in->GR[j];
                uint16_t gI = g0;
                int32_t den = horiz ? ((int32_t)yj - yi) : ((int32_t)xj - xi);

                if (den != 0)
                {
                    int32_t num = horiz ? ((int32_t)clipv - yi) : ((int32_t)clipv - xi);
                    int64_t slope_g = ((int64_t)num << 14) / den;
                    int32_t dg = (int32_t)g1 - (int32_t)g0;
                    gI = (uint16_t)(g0 + (int32_t)((dg * slope_g) >> 14));
                }

                out->v[di].x = Xc;
                out->v[di].y = Yc;
                out->GR[di] = gI;
                out->VX[di] = in->VX[i];
                di++;
            }
        }
        out->sides = (uint16_t)di;
        out->vxseg = in->vxseg;
        out->flags = in->flags;
        out->color = in->color;
    }

    static void newclip(Poly ** P, Poly * tmp, uint8_t vis_and, uint8_t vis_or)
    {
        Poly * in = *P;
        Poly * out = tmp;

        if (in->sides < 3)
        {
            in->sides = 0;
            *P = in;
            return;
        }

        if (vis_and & VF_FAR)
        {
            in->sides = 0;
            *P = in;
            return;
        }

        if (vis_or & VF_NEAR)
        {
            uint8_t inserted_2d_bits = clip_near_reproject(in, out);
            Poly * t = in;
            in = out;
            out = t;
            if (in->sides < 3)
            {
                in->sides = 0;
                *P = in;
                return;
            }

            vis_or = (uint8_t)((vis_or | inserted_2d_bits) & 0xFF);
        }
        else
        {
            // keep only 2D bits if we didn't near-clip (NEAR/FAR no longer needed below)
            vis_or &= (VF_UP | VF_DOWN | VF_LEFT | VF_RIGHT);
        }

        if (vis_or & VF_UP)
        {
            side_clip(in, out, out_up, 1, projclipy[0]);
            Poly * t = in;
            in = out;
            out = t;
            if (in->sides < 3)
            {
                in->sides = 0;
                *P = in;
                return;
            }
        }

        if (vis_or & VF_DOWN)
        {
            side_clip(in, out, out_down, 1, projclipy[1]);
            Poly * t = in;
            in = out;
            out = t;
            if (in->sides < 3)
            {
                in->sides = 0;
                *P = in;
                return;
            }
        }

        if (vis_or & VF_LEFT)
        {
            side_clip(in, out, out_left, 0, projclipx[0]);
            Poly * t = in;
            in = out;
            out = t;
            if (in->sides < 3)
            {
                in->sides = 0;
                *P = in;
                return;
            }
        }

        if (vis_or & VF_RIGHT)
        {
            side_clip(in, out, out_right, 0, projclipx[1]);
            Poly * t = in;
            in = out;
            out = t;
            if (in->sides < 3)
            {
                in->sides = 0;
                *P = in;
                return;
            }
        }

        *P = in;
    }

    void draw_polylist(polylist * L, polydata * D, vlist * V, pvlist * PV, nlist * N, int F)
    {
        polyoversample = (unsigned char)projoversampleshr;
        polyoversample16 = (unsigned char)(16 - polyoversample);
        polyoversamples = (unsigned char)(1u << polyoversample);
        polyoversample_mask = (unsigned char)(polyoversamples - 1);

        /* bail out early if object not visible */
        if ((F & F_VISIBLE) == 0) return;

        /* walk list: skip count + sortvertex (each a 16-bit word) */
        unsigned short * lp = (unsigned short *)L;
        lp += 2;

        for (;;)
        {
            unsigned short off = *lp++;
            if (off == 0) break;

            /* polydata record: list entry is a BYTE offset into D */
            unsigned char * si = (unsigned char *)D + off;

            /* header word: lo8 = sides, hi8 = per-poly flags (byte) */
            unsigned short hdr = *(unsigned short *)(si + 0);
            unsigned short sides = (unsigned short)(hdr & 0x00FF);

            // Keep only per-poly bits that are allowed by object F (plus 0x0F00 shade-quant),
            // and always keep the object's F_VISIBLE bit.
            unsigned short flags = (unsigned short)((hdr & (F | 0x0F00)) | (F & F_VISIBLE));

            /* color & sentinel; normal index (indices, not offsets) */
            unsigned short colorw = *(unsigned short *)(si + 2);
            if (colorw == 0xFFFF) continue; /* skip face */

            unsigned char color8 = (unsigned char)(colorw & 0x00FF);
            unsigned short normal_i = *(unsigned short *)(si + 4);

            /* backface cull unless two-sided */
            if ((flags & F_2SIDE) == 0)
            {
                unsigned short v0_i = *(unsigned short *)(si + 6);
                const vlist * v0 = &V[v0_i];
                const nlist * nn = &N[normal_i];

                if (culled_by_backface(nn, v0)) continue; /* carry=1, hidden */
            }

            /* gather vertices & accumulate visibility flags */
            poly1.sides = sides;
            poly1.flags = flags;
            poly1.vxseg = V;

            unsigned char anded = 0xFF; /* AND of all per-vertex VF (low byte) */
            unsigned char orred = 0x00; /* OR  of all per-vertex VF (low byte) */

            for (unsigned i = 0; i < sides; ++i)
            {
                unsigned short vi = *(unsigned short *)(si + 6 + 2 * i);

                /* 2D projected vertex + VF (index into PV[]) */
                const pvlist * pvi = &PV[vi];
                poly1.v[i].x = pvi->x; /* safe 16-bit copies */
                poly1.v[i].y = pvi->y;

                /* 3D vertex pointer (index into V[]) */
                poly1.VX[i] = &V[vi];

                /* LOW BYTE ONLY for VF */
                unsigned char vf = (unsigned char)(pvi->vf & 0xFF);
                anded &= vf;
                orred |= vf;
            }

            /* shading */
            if (flags & F_GOURAUD)
            {
                poly1.color = (unsigned short)color8; /* base palette index */

                for (unsigned i = 0; i < sides; ++i)
                {
                    unsigned short nv_i = (unsigned short)poly1.VX[i]->normal; /* index into N[] */
                    unsigned short q = calclight_quant(flags, &N[nv_i]);
                    poly1.GR[i] = (unsigned short)((((poly1.color + q) & 0xFF) << 8)); /* 8.8 */
                }
            }
            else
            {
                unsigned short q = calclight_quant(flags, &N[normal_i]);
                poly1.color = (unsigned short)((color8 + q) & 0xFF);
            }

            /* trivial reject & clipping */
            if (anded) continue;

            Poly * cur = &poly1;
            Poly * tmp = &poly2;

            if (orred)
            {
                newclip(&cur, tmp, anded, orred); /* NEAR first, then UP/DOWN/LEFT/RIGHT */

                if (cur->sides < 3) continue;
            }

            /* build stream and draw (0 = flat/nrm, 1 = gouraud) */
            if (unsigned short * out = polydrw; flags & F_GOURAUD)
            {
                *out++ = 1;
                poly_grd_build(cur, out);
            }
            else
            {
                *out++ = 0;
                poly_nrm_build(cur, out);
            }

            vid_drawfill(polydrw);
        }
    }

    void drawobject(object * o)
    {
        int a, b, c;
        int32_t al, bl;

        if (!(o->flags & F_VISIBLE)) return;
        calc_rotate(o->vnum, o->v, o->v0, o->r);
        if (o->flags & F_GOURAUD)
            calc_nrotate(o->nnum, o->n, o->n0, o->r);
        else
            calc_nrotate(o->nnum1, o->n, o->n0, o->r);
        o->vf = static_cast<visfl>(calc_project(o->vnum, o->pv, o->v));

        if (o->vf) return; // object was completely out of screen

        a = 0;
        al = 0x7fffffffL;

        for (b = 1; b < o->plnum; b++)
        {
            c = o->pl[b][1];
            bl = (o->v + c)->z;
            if (bl < al)
            {
                al = bl;
                a = b;
            }
        }

        draw_polylist(o->pl[a], o->pd, o->v, o->pv, o->n, o->flags);
    }

    int32_t lsget(unsigned char f)
    {
        int32_t l = 0;

        switch (f & 3)
        {
            case 0:
                l = 0;
                break;
            case 1:
                l = (int32_t)(char)(*sp++);
                break;
            case 2:
                l = *sp++;
                l |= (int32_t)(char)(*sp++) << 8;
                break;
            case 3:
                l = *sp++;
                l |= (int32_t)(*sp++) << 8;
                l |= (int32_t)(*sp++) << 16;
                l |= (int32_t)(char)(*sp++) << 24;
                break;
        }

        return (l);
    }

    void resetscene()
    {
        sp = (unsigned char *)(scenelist[sclp].data);

        for (int index = 0; index < conum; index++)
        {
            memset(co[index].o->r, 0, sizeof(rmatrix));
            memset(co[index].o->r0, 0, sizeof(rmatrix));
        }

        if (sclp++; sclp >= scl)
        {
            sclp = 0;
        }
    }

    char * readfile(char * name)
    {
        Blob::Handle f1 = Blob::open(name);

        Blob::seek(f1, 0L, SEEK_END);

        const int32_t size = Blob::tell(f1);

        char * p = reinterpret_cast<char *>(memPool);

        memPool += size;

        Blob::seek(f1, 0L, SEEK_SET);

        Blob::read(p, (size_t)size, 1, f1);

        return p;
    }
}
