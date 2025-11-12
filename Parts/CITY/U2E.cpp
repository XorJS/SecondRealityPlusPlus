#define _CRT_SECURE_NO_WARNINGS

#include "Parts/VISU/VISU.h"

namespace U2E
{
    namespace
    {
        const char u2e_scene[64] = { "U2E" };

        Palette fpal{};

        int repeat{};
    }

    void u2e_copper2()
    {
        Visu::syncframe++;

        if (Visu::cl[0].ready == 2) Visu::cl[0].ready = 0;
        if (Visu::cl[1].ready == 2) Visu::cl[1].ready = 0;
        if (Visu::cl[2].ready == 2) Visu::cl[2].ready = 0;
        if (Visu::cl[3].ready == 2) Visu::cl[3].ready = 0;

        Visu::deadlock++;
        Visu::coppercnt++;

        if (Visu::copperdelay > 0)
        {
            Visu::copperdelay--;
        }

        if (Visu::copperdelay > 0) return;

        Visu::copperdelay = 0;

        if (Visu::cl[Visu::clr].ready)
        {
            Visu::cl[(Visu::clr - 1) & 3].ready = 2;

            Visu::copperdelay = Visu::cl[Visu::clr].frames;
            Visu::clr++;
            Visu::clr &= 3;
        }
        else
            Visu::avgrepeat++;
    }

    void fadeset(unsigned char * vram)
    {
        int y;

        Shim::outp(0x3c4, 2);
        Shim::outp(0x3c5, 15);

        for (y = 0; y < 25; y++)
        {
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 0, 0, 17 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 0, 0, 17 * 4);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 17 * 4, 252, 47 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 17 * 4, 252, 47 * 4);
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 2 + 4 + 8);
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 1) = 0;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 2) = 0;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 3) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 1) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 2) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 3) = 0;
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 15);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 252 * 4, 0, 16 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 252 * 4, 0, 16 * 4);
        }

        for (y = 25; y < 175; y++)
        {
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 0, 254, 17 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 0, 254, 17 * 4);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 17 * 4, 253, 47 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 17 * 4, 253, 47 * 4);
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 2 + 4 + 8);
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 1) = 254;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 2) = 254;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 3) = 254;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 1) = 254;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 2) = 254;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 3) = 254;
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 15);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 64 * 4, 254, 16 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 64 * 4, 254, 16 * 4);
        }

        for (y = 175; y < SCREEN_HEIGHT; y++)
        {
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 0, 0, 17 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 0, 0, 17 * 4);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 17 * 4, 252, 47 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 17 * 4, 252, 47 * 4);
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 2 + 4 + 8);
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 1) = 0;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 2) = 0;
            *(vram + (y * 2 + 0) * SCREEN_WIDTH + 63 * 4 + 3) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 1) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 2) = 0;
            *(vram + (y * 2 + 1) * SCREEN_WIDTH + 63 * 4 + 3) = 0;
            Shim::outp(0x3c4, 2);
            Shim::outp(0x3c5, 15);
            memset(vram + (y * 2 + 0) * SCREEN_WIDTH + 64 * 4, 0, 16 * 4);
            memset(vram + (y * 2 + 1) * SCREEN_WIDTH + 64 * 4, 0, 16 * 4);
        }
    }

    void main()
    {
        CLEAR(fpal);
        repeat = {};

        Visu::reset();

        char * cp;

        int a, b, c, d, e, f, g;

        sprintf(Visu::tmpname, "%s.00M", u2e_scene);

        Visu::scene0 = Visu::scenem = Visu::readfile(Visu::tmpname);

        if (Visu::scene0[15] == 'C') Visu::city = 1;
        if (Visu::scene0[15] == 'R') Visu::city = 2;

        short * ip = (short *)(Visu::scene0 + LONGAT(Visu::scene0 + 4));

        Visu::conum = d = *ip++;

        for (f = -1, c = 1; c < d; c++)
        {
            if (e = *ip++; e > f)
            {
                f = e;
                sprintf(Visu::tmpname, "%s.%03i", u2e_scene, e);

                Visu::co[c].o = Visu::loadobject(Visu::tmpname);
                memset(Visu::co[c].o->r, 0, sizeof(Visu::rmatrix));
                memset(Visu::co[c].o->r0, 0, sizeof(Visu::rmatrix));
                Visu::co[c].index = e;
                Visu::co[c].on = 0;
            }
            else
            {
                for (g = 0; g < c; g++)
                    if (Visu::co[g].index == e) break;

                memcpy(Visu::co + c, Visu::co + g, sizeof(Visu::s_co));
                Visu::co[c].o = Visu::getNewObject();
                memcpy(Visu::co[c].o, Visu::co[g].o, sizeof(Visu::object));
                Visu::co[c].o->r = Visu::getNewRMatrix();
                Visu::co[c].o->r0 = Visu::getNewRMatrix();

                memset(Visu::co[c].o->r, 0, sizeof(Visu::rmatrix));
                memset(Visu::co[c].o->r0, 0, sizeof(Visu::rmatrix));

                Visu::co[c].on = 0;
            }
        }

        Visu::co[0].o = &Visu::camobject;
        Visu::camobject.r = &Visu::cam;
        Visu::camobject.r0 = &Visu::cam;

        sprintf(Visu::tmpname, "%s.0AA", u2e_scene);

        ip = (short *)Visu::readfile(Visu::tmpname);
        Visu::scl = 0;

        while (*ip)
        {
            a = *ip;
            if (a == -1) break;
            sprintf(Visu::tmpname, "%s.0%c%c", u2e_scene, a / 10 + 'A', a % 10 + 'A');

            Visu::scenelist[Visu::scl].data = Visu::readfile(Visu::tmpname);

            Visu::scl++;
            ip += 2;
        }

        Visu::resetscene();

        Shim::outp(0x3c7, 0);

        for (a = 0; a < PaletteByteCount; a++)
            fpal[a] = Shim::inp(0x3c9);

        for (b = 0; b < 33; b++)
        {
            for (a = 3; a < PaletteByteCount - 6; a++)
            {
                fpal[a] += 2;
                if (fpal[a] > 63) fpal[a] = 63;
            }

            demo_vsync();

            Shim::outp(0x3c8, 0);

            for (a = 0; a < PaletteByteCount; a++)
                Shim::outp(0x3c9, fpal[a]);

            demo_blit();
        }

        for (b = 0; b < 16; b++)
        {
            demo_vsync(true);
        }

        fadeset(Shim::vram);

        for (b = 0; b < 16; b++)
        {
            demo_vsync(true);
        }

        for (b = 0; b < 33; b++)
        {
            for (a = 3; a < PaletteByteCount - 9; a++)
            {
                fpal[a] -= 2;
                if (fpal[a] < 0) fpal[a] = 0;
            }

            for (a = PaletteByteCount - 9; a < PaletteByteCount - 3; a++)
            {
                fpal[a] += 2;
                if (fpal[a] > 63) fpal[a] = 63;
            }

            demo_vsync();

            Shim::outp(0x3c8, 0);

            for (a = 0; a < PaletteByteCount; a++)
                Shim::outp(0x3c9, fpal[a]);

            demo_blit();
        }

        demo_changemode(SCREEN_WIDTH, SCREEN_HEIGHT);

        Visu::init();

        cp = (char *)(Visu::scenem + 16);
        cp[255 * 3 + 0] = 0;
        cp[255 * 3 + 1] = 0;
        cp[255 * 3 + 2] = 0;
        cp[252 * 3 + 0] = 0;
        cp[252 * 3 + 1] = 0;
        cp[252 * 3 + 2] = 0;
        cp[253 * 3 + 0] = 63;
        cp[253 * 3 + 1] = 63;
        cp[253 * 3 + 2] = 63;
        cp[254 * 3 + 0] = 63;
        cp[254 * 3 + 1] = 63;
        cp[254 * 3 + 2] = 63;

        Common::setpalarea(cp);

        Visu::window(0L, 319L, 25L, 174L, 512L, 9999999L);

        Shim::clearScreen();

        Visu::xit = 0;
        Visu::currframe = 0;

        while (!demo_wantstoquit())
        {
            a = Music::getOrder();

            if (a > 18) break;

            AudioPlayer::Update(true);
        }

        Visu::coppercnt = 0;
        Visu::syncframe = 0;
        Visu::avgrepeat = 1;
        Visu::cl[0].ready = 0;
        Visu::cl[1].ready = 0;
        Visu::cl[2].ready = 0;
        Visu::cl[3].ready = 1;

        int fov = 0;
        while (!demo_wantstoquit() && !Visu::xit)
        {
            int onum;
            int32_t pflag;
            int32_t dis;
            int32_t l;

            Visu::object * o;
            Visu::rmatrix * r;

            memset(Shim::vram, 0, SCREEN_WIDTH * SCREEN_HEIGHT);

            if (!Visu::firstframe)
            {
                Visu::deadlock = 0;

                Visu::clear255();

                // Field of vision
                Visu::cameraangle(static_cast<Visu::angle>(fov));

                // Calc matrices and add to order list (only enabled objects)
                Visu::ordernum = 0;

                /* start at 1 to skip camera */
                for (a = 1; a < Visu::conum; a++)
                    if (Visu::co[a].on)
                    {
                        Visu::order[Visu::ordernum++] = a;
                        o = Visu::co[a].o;
                        memcpy(o->r, o->r0, sizeof(Visu::rmatrix));
                        Visu::calc_applyrmatrix(o->r, &Visu::cam);

                        b = o->pl[0][1]; // center vertex

                        if (Visu::co[a].o->name[1] == '_')
                            Visu::co[a].dist = 1000000000L;
                        else
                            Visu::co[a].dist = Visu::calc_singlez(b, o->v0, o->r);

                        if (Visu::currframe > 900 * 2 && Visu::currframe < 1100 * 2)
                        {
                            if (Visu::co[a].o->name[1] == 's' && Visu::co[a].o->name[2] == '0' && Visu::co[a].o->name[3] == '1') Visu::co[a].dist = 1L;
                        }
                    }

                for (a = 0; a < Visu::ordernum; a++)
                {
                    dis = Visu::co[c = Visu::order[a]].dist;
                    for (b = a - 1; b >= 0 && dis > Visu::co[Visu::order[b]].dist; b--)
                        Visu::order[b + 1] = Visu::order[b];
                    Visu::order[b + 1] = c;
                }

                // Draw
                for (a = 0; a < Visu::ordernum; a++)
                {
                    o = Visu::co[Visu::order[a]].o;

                    Visu::drawobject(o);
                }

                // **** Drawing completed **** //
            }
            else
            {
                Visu::syncframe = 0;
                Visu::firstframe = 0;
                Visu::coppercnt = 1;
            }

            // calculate how many frames late of schedule

            a = (Visu::syncframe - Visu::currframe);
            repeat = a + 1;
            if (repeat < 0) repeat = 0;
            if (repeat == 0)
                Visu::cl[Visu::clw].frames = 1;
            else
                Visu::cl[Visu::clw].frames = repeat;
            Visu::cl[Visu::clw].ready = 1;
            Visu::clw++;
            Visu::clw &= 3;

            // advance that many frames
            repeat = (repeat + 1) / 2;
            Visu::currframe += repeat * 2;

            while (repeat-- && !Visu::xit)
            {
                // parse animation stream for 1 frame
                onum = 0;
                while (!Visu::xit)
                {
                    a = *Visu::sp++;
                    if (a == 0xff)
                    {
                        a = *Visu::sp++;
                        if (a <= 0x7f)
                        {
                            fov = a << 8;
                            break;
                        }
                        else if (a == 0xff)
                        {
                            Visu::resetscene();
                            Visu::xit = 1;
                            continue;
                        }
                    }
                    if ((a & 0xc0) == 0xc0)
                    {
                        onum = ((a & 0x3f) << 4);
                        a = *Visu::sp++;
                    }
                    onum = (onum & 0xff0) | (a & 0xf);
                    b = 0;
                    switch (a & 0xc0)
                    {
                        case 0x80:
                            b = 1;
                            Visu::co[onum].on = 1;
                            break;
                        case 0x40:
                            b = 1;
                            Visu::co[onum].on = 0;
                            break;
                    }

                    if (onum >= Visu::conum)
                    {
                        return;
                    }

                    r = Visu::co[onum].o->r0;

                    pflag = 0;
                    switch (a & 0x30)
                    {
                        case 0x00:
                            break;
                        case 0x10:
                            pflag |= *Visu::sp++;
                            break;
                        case 0x20:
                            pflag |= Visu::sp[0];
                            pflag |= (int32_t)Visu::sp[1] << 8;
                            Visu::sp += 2;
                            break;
                        case 0x30:
                            pflag |= Visu::sp[0];
                            pflag |= (int32_t)Visu::sp[1] << 8;
                            pflag |= (int32_t)Visu::sp[2] << 16;
                            Visu::sp += 3;
                            break;
                    }

                    l = Visu::lsget(static_cast<unsigned char>(pflag));
                    r->x += l;
                    l = Visu::lsget(static_cast<unsigned char>(pflag >> 2));
                    r->y += l;
                    l = Visu::lsget(static_cast<unsigned char>(pflag >> 4));
                    r->z += l;

                    if (pflag & 0x40)
                    { // word matrix
                        for (b = 0; b < 9; b++)
                            if (pflag & (0x80 << b))
                            {
                                r->m[b] += Visu::lsget(2);
                            }
                    }
                    else
                    { // byte matrix
                        for (b = 0; b < 9; b++)
                            if (pflag & (0x80 << b))
                            {
                                r->m[b] += Visu::lsget(1);
                            }
                    }
                }
            }

            demo_blit();

            demo_vsync();

            u2e_copper2();
        }

        Shim::outp(0x3c7, 0);

        for (a = 0; a < PaletteByteCount; a++)
            fpal[a] = Shim::inp(0x3c9);

        for (b = 0; b < 16; b++)
        {
            for (a = 0; a < PaletteByteCount; a++)
            {
                fpal[a] += 4;
                if (fpal[a] > 63) fpal[a] = 63;
            }

            demo_vsync();

            Shim::outp(0x3c8, 255);

            for (a = 0; a < PaletteByteCount; a++)
                Shim::outp(0x3c9, fpal[a]);

            demo_blit();
        }
    }

}
