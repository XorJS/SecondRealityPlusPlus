#pragma once

#include <stdio.h>

#include "Parts/Common.h"

#define LONGAT(zz) *((int32_t *)(zz))
#define INTAT(zz) *((short *)(zz))
#define CHARAT(zz) *((char *)(zz))

namespace Visu
{
    constexpr const size_t MAXOBJ = 256;

    using angle = unsigned short; /* 0..65535, 65536=360 degrees */
    using visfl = short;

    struct rmatrix /* special matrix specifying position & rotation */
    {
        int32_t m[9];
        int32_t x;
        int32_t y;
        int32_t z;
    };

    struct vlist
    {
        int32_t x;
        int32_t y;
        int32_t z;
        short normal;
        short RESERVED;
    };

    struct nlist
    {
        short x;
        short y;
        short z;
        short RESERVED;
    };

    struct pvlist
    {
        short x;
        short y;
        visfl vf; // if vf&VF_NEAR, the x/y are undefined
        short RESERVED[5];
    };

    using polylist = short;
    /* polylist contents:
        word: number of words in list (including last 0)
        word: sort polygon for this list
        word: pointer to polygon 1 inside polydata
        word: pointer to polygon 2 inside polydata
        word: pointer to polygon 3 inside polydata
        ...
        word: 0 = end of list */

    using polydata = char;
    /* polydata consists of first a zero word, following by variable
       length records:
    byte    sides (=n)
    byte    flags (=8 lower bits of PF flags)
    byte    color
    byte    RESERVED
    word    normal (index inside normal vertices list)
    word    vertex 1
    word    vertex 2
    ...
    word    vertex n
    Total length for one record is: 4+2*n
    */

    struct object
    {
        short flags;            /* flags */
        struct object * parent; /* parent object */
        rmatrix * r0;           /* rotation/position relative to parent */
        rmatrix * r;            /* rotation/position (modified by camera etc from r0)*/
        polydata * pd;          /* polygon data block */
        short pdlen;            /* length of polygon data block in bytes */
        short plnum;            /* number of pl-lists */
        short * pl[16];         /* unsorted order (0) + precalculated polygon
                            lists from max 16 directions (1..8). List contents:
                            word: length in words
                            word: closest vertex (for list 0, center vertex)
                            word: 1st polygon
                            word: 2nd polygon
                            ...
                            word: 0 */
        short vnum;             /* number of vertices */
        short nnum;             /* number of normals */
        short nnum1;            /* number of basic normals. Normals betwen nnum1..nnum
                        are for gouraud shading */
        vlist * v0;             /* original vertices. */
        nlist * n0;             /* original normals */
        vlist * v;              /* calced: rotated vertices */
        nlist * n;              /* calced: rotated normals */
        pvlist * pv;            /* calced: projected vertices */
        visfl vf;               /* calced: visibility flag for entire pointlist (log.and) */
        char * name;            /* asciiz name for object */
    };

    struct s_co
    {
        object * o;
        int32_t dist;
        int index;
        int on;
    };

    struct s_scl
    {
        char * data;
    };

    struct s_cl
    {
        int frames;
        int ready; // 1=to be displayed, 0=displayed
    };

    void reset();

    object * getNewObject();
    rmatrix * getNewRMatrix();

    void init();

    object * loadobject(char * fname);

    void drawobject(object * o);

    void window(int32_t x1, int32_t x2, int32_t y1, int32_t y2, int32_t z1, int32_t z2);
    void cameraangle(angle a);

    void clearbg(char *);
    void clear255();

    void calc_applyrmatrix(rmatrix * dest, rmatrix * apply);
    int32_t calc_singlez(int vertexnum, vlist * vertexlist, rmatrix * matrix);

    int32_t lsget(unsigned char f);

    void resetscene();

    char * readfile(char * name);

    extern char tmpname[64];
    extern char * scene0;
    extern char * scenem;

    extern int city;
    extern int xit;

    extern s_scl scenelist[64];
    extern int scl, sclp;
    extern s_co co[MAXOBJ];
    extern int conum;

    extern char fpal[PaletteByteCount];
    extern object camobject;
    extern rmatrix cam;

    extern int order[MAXOBJ], ordernum;
    extern unsigned char * sp;

    extern s_cl cl[4];
    extern int clr, clw;
    extern int firstframe;
    extern int deadlock;
    extern int coppercnt;
    extern int syncframe;
    extern int currframe;
    extern int copperdelay;
    extern int repeat, avgrepeat;
}