#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <chrono>

#include "Graphics/Graphics.h"
#include "Parts/Common.h"

#define FORWARD_PART(X) \
    namespace X         \
    {                   \
        void main();    \
    }

FORWARD_PART(Alku)
FORWARD_PART(U2A)
FORWARD_PART(OUTTA)
FORWARD_PART(Beg)
FORWARD_PART(Glenz)
FORWARD_PART(Tunneli)
FORWARD_PART(KOE)
FORWARD_PART(Shutdown)
FORWARD_PART(Forest)
FORWARD_PART(Lens)
FORWARD_PART(PLZ)
FORWARD_PART(Dots)
FORWARD_PART(Water)
FORWARD_PART(Coman)
FORWARD_PART(JPLogo)
FORWARD_PART(U2E)
FORWARD_PART(End)
FORWARD_PART(Credits)
FORWARD_PART(EndScrl)
FORWARD_PART(DDStars)

namespace
{
    enum struct Part
    {
        _00_AlkutekstitI,
        _01_AlkutekstitII,
        _02_AlkutekstitIII,
        _03_Logo,
        _04_Glenz,
        _05_Dottitunneli,
        _06_Techno,
        _07_Panicfake,
        _08_VuoriScrolli,
        _09_Rotazoomer,
        _10_Plasmacube,
        _11_MiniVectorBalls,
        _12_Peilipalloscroll,
        _13_3D_Sinusfield,
        _14_Jellypic,
        _15_VectorPartII,
        _16_Endpictureflash,
        _17_CreditsGreetings,
        _18_EndScrolling,
        ____Empty0,
        _19_HiddenPart,
        ____Empty1,
    };

    struct PartInfo
    {
        Music::Song music;
        unsigned char musicStartOrder;
        unsigned short width;
        unsigned short height;
        void (*part)() = nullptr;
    };

    const PartInfo main_parts[] = {
        /* 00 Alkutekstit I     */ { Music::Song::Skaven, 0x00, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Alku::main },
        /* 01 Alkutekstit II    */ { Music::Song::Skaven, 0x0C, SCREEN_WIDTH, SCREEN_HEIGHT, U2A::main },
        /* 02 Alkutekstit III   */ { Music::Song::Skaven, 0x0D, SCREEN_WIDTH, SCREEN_HEIGHT, OUTTA::main },
        /* 03 Logo              */ { Music::Song::Skaven, 0x0E, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Beg::main },
        /* 04 Glenz             */ { Music::Song::PurpleMotion, 0x00, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Glenz::main },
        /* 05 Dottitunneli      */ { Music::Song::PurpleMotion, 0x0F, SCREEN_WIDTH, SCREEN_HEIGHT, Tunneli::main },
        /* 06 Techno            */ { Music::Song::PurpleMotion, 0x14, SCREEN_WIDTH, SCREEN_HEIGHT, KOE::main },
        /* 07 Panicfake         */ { Music::Song::PurpleMotion, 0x27, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Shutdown::main },
        /* 08 Vuori-Scrolli     */ { Music::Song::PurpleMotion, 0x2A, SCREEN_WIDTH, SCREEN_HEIGHT, Forest::main },
        // Lens
        /* 09 Rotazoomer        */ { Music::Song::PurpleMotion, 0x2F, SCREEN_WIDTH, SCREEN_HEIGHT, Lens::main },
        // Plasma
        /* 10 Plasmacube        */ { Music::Song::PurpleMotion, 0x3E, SCREEN_WIDTH, SCREEN_HEIGHT, PLZ::main },
        /* 11 MiniVectorBalls   */ { Music::Song::PurpleMotion, 0x4D, SCREEN_WIDTH, SCREEN_HEIGHT, Dots::main },
        /* 12 Peilipalloscroll  */ { Music::Song::PurpleMotion, 0x58, SCREEN_WIDTH, SCREEN_HEIGHT, Water::main },
        /* 13 3D-Sinusfield     */ { Music::Song::PurpleMotion, 0x5E, SCREEN_WIDTH, SCREEN_HEIGHT, Coman::main },
        /* 14 Jellypic          */ { Music::Song::PurpleMotion, 0x62, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, JPLogo::main },
        /* 15 Vector Part II    */ { Music::Song::Skaven, 0x12, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, U2E::main },
        /* 16 Endpictureflash   */ { Music::Song::Skaven, 0x19, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, End::main },
        /* 17 Credits/Greetings */ { Music::Song::Skaven, 0x1C, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, Credits::main },
        /* 18 EndScrolling      */ { Music::Song::Skaven, 0x2B, DOUBlE_SCREEN_WIDTH, 350, EndScrl::main },
        { Music::Song::COUNT, 0x00, 0, 0, nullptr },
        /* 09 HiddenPart        */ { Music::Song::Skaven, 0x46, SCREEN_WIDTH, DOUBlE_SCREEN_HEIGHT, DDStars::main },
        { Music::Song::COUNT, 0x00, 0, 0, nullptr },
    };

    Graphics g_graphics;

    float g_lastVblank = { 0.0f };

#if defined(_DEBUG) || !defined(_WIN32)
    bool g_windowMode = true;
#else
    bool g_windowMode = false;
#endif // _DEBUG

    bool g_looping = false;

    Part g_start = Part::_00_AlkutekstitI;
}

void demo_changemode(int x, int y, float jsss)
{
    g_graphics.ChangeMode(x, y, jsss);
}

void demo_blit()
{
    g_graphics.Update();
}

bool demo_wantstoquit()
{
    return g_graphics.WantsToQuit();
}

float get_time_ms_precise()
{
    static std::chrono::time_point<std::chrono::high_resolution_clock> startTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsedSeconds = std::chrono::high_resolution_clock::now() - startTime;

    return static_cast<float>(elapsedSeconds.count() * 1000.0f);
}

int demo_vsync(bool updateAudio)
{
    if (updateAudio)
    {
        AudioPlayer::Update(true);
    }

    const float cycle_ms = 1000.0f / 70.0f;
    float now = get_time_ms_precise();
    float elapsed = now - g_lastVblank;

    while (elapsed < cycle_ms)
    {
        now = get_time_ms_precise();
        elapsed = now - g_lastVblank;
    }

    g_lastVblank = now;

    return 1;
}

void mainLoop()
{
    Music::Song lastMusic = Music::Song::COUNT;

    do
    {
        for (int index = static_cast<int>(g_start); main_parts[index].width; index++)
        {
            if (lastMusic != main_parts[index].music)
            {
                lastMusic = main_parts[index].music;

                Music::start(main_parts[index].music, main_parts[index].musicStartOrder);
            }

            demo_changemode(main_parts[index].width, main_parts[index].height);

            main_parts[index].part();

            if (demo_wantstoquit())
            {
                break;
            }

            Shim::finishedDemoFirstPart();

            if (index == static_cast<int>(Part::_07_Panicfake))
            {
                // wait for music between the shutdown / forest parts
                while (!demo_wantstoquit() && Music::getPlusFlags() >= 0)
                {
                    demo_vsync();
                    demo_blit();
                }

                while (!demo_wantstoquit() && Music::getPlusFlags() < 0)
                {
                    demo_vsync();
                    demo_blit();
                }
            }

            // loop
            if (g_looping && index == static_cast<int>(Part::_17_CreditsGreetings))
            {
                lastMusic = Music::Song::COUNT;
                index = -1;
            }
        }

        if (demo_wantstoquit())
        {
            break;
        }

    } while (g_looping);
}

#ifndef _WIN32
const int MB_ICONINFORMATION = 0;
void MessageBoxA(void * /*hWnd*/, const char * text, const char * caption, int /*type*/)
{
    printf("%s\n\n%s", caption, text);
}
#endif

int main(int argc, char * argv[])
{
    if (argc > 1)
    {
        bool invalidArg = false;

        for (auto index = 1; index < argc; index++)
        {
            switch (argv[index][0])
            {
                case '2':
                    g_start = Part::_03_Logo;
                    break;
                case '3':
                    g_start = Part::_08_VuoriScrolli;
                    break;
                case '4':
                    g_start = Part::_15_VectorPartII;
                    break;
                case '5':
                    g_start = Part::_18_EndScrolling;
                    break;
                case 'u':
                case 'U': {
                    g_start = Part::_19_HiddenPart;
                    break;
                }
                case 'l':
                case 'L': {
                    g_looping = true;
                    break;
                }
                case 'w':
                case 'W': {
                    g_windowMode = true;
                    break;
                }
                case 'f':
                case 'F': {
                    g_windowMode = false;
                    break;
                }
                default: {
                    invalidArg = true;
                    break;
                }
            }
        }

        if (invalidArg)
        {
            const char * parameterList = "Parameter List:\n\n"
                                         "2: Logo\n"
                                         "3: Vuori Scrolli\n"
                                         "4: Vector Part II\n"
                                         "5: End Scrolling\n\n"
                                         "U: Hidden Part\n\n"
                                         "L: Looping\n\n"
                                         "W: Window Mode\n"
                                         "F: Fullscreen Mode\n";

            MessageBoxA(nullptr, parameterList, "Second Reality++ - Invalid Parameter(s)", MB_ICONINFORMATION);

            return 0;
        }
    }

    if (!g_graphics.Init(g_windowMode ? Graphics::WindowType::Windowed : Graphics::WindowType::Fullscreen))
    {
        return -3;
    }

#ifndef __EMSCRIPTEN__
    mainLoop();
#else
    emscripten_set_main_loop(mainLoop, 0, 1);
    emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
#endif

    Music::end();

    g_graphics.Close();

    return 0;
}
