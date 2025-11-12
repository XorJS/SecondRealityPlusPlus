#include "Parts/Common.h"

#include "END_Data.h"

namespace End
{
    namespace
    {
        Palette pal2{};
        Palette palette{};
    }

    void main()
    {
        CLEAR(pal2);
        CLEAR(palette);

        int a, c, y;

        Shim::clearScreen();

        demo_vsync();

        Shim::outp(0x3c8, 0);

        for (a = 0; a < PaletteByteCount - 3; a++)
            Shim::outp(0x3c9, 63);

        demo_vsync();

        Shim::outp(0x3c8, 0);

        for (a = 0; a < PaletteByteCount - 3; a++)
            Shim::outp(0x3c9, 63);

        for (a = 0; a < 32; a++)
        {
            demo_vsync(true);
        }

        Common::readp(palette, -1, (const char *)Data::end_pic);

        for (y = 0; y < DOUBlE_SCREEN_HEIGHT; y++)
        {
            Common::readp((char *)(Shim::vram + y * SCREEN_WIDTH), y, (const char *)Data::end_pic);
        }

        for (c = 0; c <= 128; c++)
        {
            for (a = 0; a < PaletteByteCount - 3; a++)
                pal2[a] = static_cast<char>(((128 - c) * 63 + palette[a] * c) / 128);

            demo_vsync();

            Common::setpalarea(pal2, 0, PaletteColorCount - 1);

            demo_blit();
        }

        for (a = 0; a < 5000 && !demo_wantstoquit(); a++)
        {
            demo_vsync();

            demo_blit();

            if (Music::getPlusFlags() > -16) break;
        }

        for (c = 63; c >= 0; c--)
        {
            for (a = 0; a < PaletteByteCount - 3; a++)
                pal2[a] = static_cast<char>((palette[a] * c) / 64);

            demo_vsync();

            Common::setpalarea(pal2, 0, PaletteColorCount - 1);

            demo_blit();
        }
    }
}