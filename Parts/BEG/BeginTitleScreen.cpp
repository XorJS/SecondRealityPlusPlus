#include "Parts/Common.h"

#include "BeginTitleScreen_Data.h"

namespace Beg
{
    namespace
    {
        Palette pal2{};
        Palette palette{};
    }

    void main()
    {
        Shim::clearScreen();

        for (int index = 0; index < 32; index++)
        {
            demo_vsync(true);
        }

        Shim::outp(0x3c8, 0);

        for (int index = 0; index < 255; index++)
        {
            Shim::outp(0x3c9, 63);
            Shim::outp(0x3c9, 63);
            Shim::outp(0x3c9, 63);
        }

        Shim::outp(0x3c9, 0);
        Shim::outp(0x3c9, 0);
        Shim::outp(0x3c9, 0);

        Common::readp(palette, -1, (char *)Data::TitleScreenData);

        for (int y = 0; y < DOUBlE_SCREEN_HEIGHT; y++)
        {
            Common::readp((char *)(Shim::vram + (unsigned)y * 320U), y, (char *)Data::TitleScreenData);
        }

        for (int c = 0; c <= 128; c++)
        {
            for (int index = 0; index < PaletteByteCount - 3; index++)
            {
                pal2[index] = static_cast<char>(((128 - c) * 63 + palette[index] * c) / 128);
            }

            demo_vsync();

            Common::setpalarea(pal2, 0, 254);
            demo_blit();
        }

        Common::setpalarea(palette, 0, 254);
    }

}