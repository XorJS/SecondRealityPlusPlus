#include "Parts/Common.h"

#include "ENDSCRL_MAIN_Data.h"

namespace EndScrl
{
    constexpr const size_t FONAY = 25;

    namespace
    {
        const char * fonaorder = "ABCDEFGHIJKLMNOPQRSTUVWXabcdefghijklmnopqrstuvwxyz0123456789!?,.:"
                                 "\x8F\x8F"
                                 "()+-*='Z"
                                 "\x84\x94"
                                 "Y/&";

        char text[64000] = "Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0Testing Testing\0PIXEL "
                           "SUCKS\0ABCDEFGHIJKLMNOPQRSTUVWXYZ\0abcdefghijklmnopqrsuvwxyz\0";

        int fonap[256]{};
        int fonaw[256]{};

        char * tptr = text;
        int tstart = 0, chars = 0;

        char textline[100]{};
        char scanbuf[640]{};
    }

    void setstart(int y)
    {
        Shim::setstartpixel(y);
    }

    void setrgbpalette(int p, int r, int g, int b)
    {
        Shim::setpal(p, static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b));
    }

    void do_scroll()
    {
        static int yscrl = 0;
        static int line = 0;

        int a, b, x;

        if (line == 0)
        {
            for (a = 0, tstart = 0, chars = 0; *tptr != '\n'; a++, chars++)
            {
                textline[a] = *tptr;
                tstart += fonaw[(unsigned char)*tptr++] + 2;
            }

            textline[a] = *tptr++;
            tstart = (639 - tstart) / 2;

            if (textline[0] == '[')
            {
                chars = 0;
            }
        }

        memset(scanbuf, 0, PLANAR_WIDTH * 4);

        for (a = 0, x = tstart; a < chars; a++, x += 2)
            for (b = 0; b < fonaw[(unsigned char)textline[a]]; b++, x++)
            {
                scanbuf[x] = Data::endscrl_font[line][fonap[(unsigned char)textline[a]] + b];
            }

        memcpy(Shim::vram + 640 * (yscrl), scanbuf, 640);
        memcpy(Shim::vram + 640 * (yscrl + 401), scanbuf, 640);

        yscrl = (yscrl + 1) % 401;

        if (textline[0] == '[')
        {
            int height = (textline[1] - '0') * 10 + (textline[2] - '0');
            line = (line + 1) % height;
        }
        else
        {
            line = (line + 1) % FONAY;
        }

        setstart(yscrl * 640);

        if (textline[0] == '%')
        {
            tptr = text;
        }
    }

    void init()
    {
        int x, y, b;

        Blob::Handle a = Blob::open("endscrol.txt");
        Blob::read(text, 60000, 1, a);

        for (x = 0; x < 1550 && *fonaorder;)
        {
            while (x < 1550)
            {
                for (y = 0; y < static_cast<int>(FONAY); y++)
                    if (Data::endscrl_font[y][x]) break;
                if (y != FONAY) break;
                x++;
            }

            b = x;

            while (x < 1550)
            {
                for (y = 0; y < static_cast<int>(FONAY); y++)
                    if (Data::endscrl_font[y][x]) break;
                if (y == FONAY) break;
                x++;
            }

            fonap[(unsigned char)*fonaorder] = b;
            fonaw[(unsigned char)*fonaorder] = x - b;
            fonaorder++;
        }

        fonap[32] = 1550 - 20;
        fonaw[32] = 16;
    }

    void main()
    {
        Shim::clearScreen();

        demo_vsync();

        setrgbpalette(1, 20, 20, 20);
        setrgbpalette(2, 40, 40, 40);
        setrgbpalette(3, 60, 60, 60);
        setrgbpalette(4, 60, 60, 60);
        setrgbpalette(5, 60, 60, 60);
        setrgbpalette(6, 60, 60, 60);
        setrgbpalette(7, 60, 60, 60);
        setrgbpalette(8, 60, 60, 60);
        setrgbpalette(9, 60, 60, 60);
        setrgbpalette(10, 60, 60, 60);
        setrgbpalette(11, 60, 60, 60);
        setrgbpalette(12, 60, 60, 60);
        setrgbpalette(13, 60, 60, 60);
        setrgbpalette(14, 60, 60, 60);
        setrgbpalette(15, 60, 60, 60);

        init();

        while (!demo_wantstoquit())
        {
            if (demo_vsync() == 1) demo_vsync();

            do_scroll();

            demo_blit();
        }
    }
}