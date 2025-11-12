#include "audioPlayer.h"

#include <stdlib.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <mmsystem.h>

#else

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#define CALLBACK

typedef uint32_t DWORD;
typedef uintptr_t DWORD_PTR;
typedef unsigned int UINT;
typedef unsigned int MMRESULT;
typedef int32_t BOOL;

typedef char CHAR;
typedef CHAR * LPSTR;

#define WAVE_MAPPER ((UINT) - 1)

#define MM_WOM_DONE 0x3BD

#define CALLBACK_FUNCTION 0x00030000l /* dwCallback is a FARPROC */

// Return codes (subset)
#ifndef MMSYSERR_NOERROR
#define MMSYSERR_NOERROR 0
#endif
#ifndef MMSYSERR_ERROR
#define MMSYSERR_ERROR 1
#endif
#ifndef WAVERR_STILLPLAYING
#define WAVERR_STILLPLAYING 33
#endif

// TIME_* for MMTIME
#ifndef TIME_MS
#define TIME_MS 0x0001
#endif
#ifndef TIME_SAMPLES
#define TIME_SAMPLES 0x0002
#endif

// WAVEHDR flags (subset)
#ifndef WHDR_DONE
#define WHDR_DONE 0x00000001
#endif
#ifndef WHDR_PREPARED
#define WHDR_PREPARED 0x00000002
#endif
#ifndef WHDR_INQUEUE
#define WHDR_INQUEUE 0x00000010
#endif

// Common PCM tags
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 0x0001
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif

typedef struct WAVEFORMATEX
{
    uint16_t wFormatTag; // 1=PCM, 3=IEEE float
    uint16_t nChannels;
    uint32_t nSamplesPerSec;
    uint32_t nAvgBytesPerSec; // nSamplesPerSec * nBlockAlign
    uint16_t nBlockAlign;     // (bits/8)*channels
    uint16_t wBitsPerSample;  // 8/16/32
    uint16_t cbSize;          // unused here
} WAVEFORMATEX, *LPWAVEFORMATEX, *LPCWAVEFORMATEX;

typedef struct WAVEHDR
{
    LPSTR lpData;
    uint32_t dwBufferLength;
    uint32_t dwBytesRecorded; // unused for output
    uint32_t dwUser;          // user field; untouched
    uint32_t dwFlags;         // WHDR_*
    uint32_t dwLoops;         // unused for output
    struct WAVEHDR * lpNext;  // unused
    uint32_t reserved;        // unused
} WAVEHDR, *LPWAVEHDR, *NPWAVEHDR, *LPWAVEHDRCONST;

typedef struct MMTIME
{
    UINT wType; // set desired type on input (TIME_MS or TIME_SAMPLES)
    union
    {
        DWORD ms;
        DWORD sample;
        DWORD cb; // (not used here)
    } u;
} MMTIME, *LPMMTIME;

struct WASM_WaveOut;
typedef WASM_WaveOut * HWAVEOUT;
typedef HWAVEOUT * LPHWAVEOUT;

struct WASM_WaveOut
{
    SDL_AudioDeviceID dev = 0;
    SDL_AudioSpec spec{};
    uint64_t bytes_submitted = 0;

    // Track buffer completion to set WHDR_DONE like WinMM.
    // Each entry keeps the header and the end offset (bytes) after queuing it.
    struct Entry
    {
        WAVEHDR * hdr;
        uint64_t end_total_bytes;
    };
    std::vector<Entry> queue;

    // Format helpers
    uint32_t frame_bytes() const
    {
        uint32_t sample_bytes = (spec.format == AUDIO_F32) ? 4 : (spec.format == AUDIO_S16) ? 2 : (spec.format == AUDIO_S8 || spec.format == AUDIO_U8) ? 1 : 2;
        return sample_bytes * spec.channels;
    }

    uint64_t bytes_played() const
    {
        uint64_t queued = SDL_GetQueuedAudioSize(dev);
        return (bytes_submitted > queued) ? (bytes_submitted - queued) : 0;
    }

    void pump_done_flags()
    {
        uint64_t played = bytes_played();

        // Mark headers as done if fully played
        while (!queue.empty() && played >= queue.front().end_total_bytes)
        {
            WAVEHDR * h = queue.front().hdr;
            if (h)
            {
                h->dwFlags &= ~WHDR_INQUEUE;
                h->dwFlags |= WHDR_DONE;
            }
            queue.erase(queue.begin());
        }
    }
};

#endif // _WIN32

#ifdef __EMSCRIPTEN__
// clang-format off
EM_JS(int, is_firefox, (), {
  const ua = navigator.userAgent || "";
  return /Firefox|FxiOS/i.test(ua) ? 1 : 0;
});
// clang-format on
#endif // __EMSCRIPTEN__

#define MIX_BUF_NUM 2

namespace AudioPlayer
{
    namespace
    {
        uint32_t g_audioFreq = 44100; // set at open time
        uint32_t g_totalSampleCount = 0;

        volatile BOOL audioRunningFlag{};

        uint8_t currBuffer{};
        int16_t * mixBuffer[MIX_BUF_NUM]{};
        WAVEHDR waveBlocks[MIX_BUF_NUM]{};
        HWAVEOUT hWave{};

#ifdef _WIN32
        HANDLE hThread{}, hAudioSem{};
#endif // _WIN32
    }

    static void CALLBACK waveProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
    {
        (void)hWaveOut;
        (void)uMsg;
        (void)dwInstance;
        (void)dwParam1;
        (void)dwParam2;

#ifdef _WIN32
        if (uMsg == WOM_DONE) ReleaseSemaphore(hAudioSem, 1, nullptr);
#endif // _WIN32
    }

#ifdef _WIN32
    static DWORD WINAPI mixThread(LPVOID lpParam)
    {
        WAVEHDR * waveBlock;

        (void)lpParam;

        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        while (audioRunningFlag)
        {
            waveBlock = &waveBlocks[currBuffer];

            st3play::FillAudioBuffer((int16_t *)waveBlock->lpData, MIX_BUF_SAMPLES);

            waveOutWrite(hWave, waveBlock, sizeof(WAVEHDR));
            currBuffer = (currBuffer + 1) % MIX_BUF_NUM;

            // wait for buffer fill request
            WaitForSingleObject(hAudioSem, INFINITE);
        }

        return 0;
    }
#else

    static SDL_AudioFormat wasm_pick_sdl_format(const WAVEFORMATEX * wfx)
    {
        if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && wfx->wBitsPerSample == 32) return AUDIO_F32;
        if (wfx->wBitsPerSample == 8) return AUDIO_U8;
        if (wfx->wBitsPerSample == 16) return AUDIO_S16;
        if (wfx->wBitsPerSample == 32) return AUDIO_F32; // common on the web

        return AUDIO_S16;
    }

    inline MMRESULT waveOutOpen(LPHWAVEOUT phwo, UINT /*uDeviceID*/, LPCWAVEFORMATEX pwfx, DWORD_PTR /*dwCallback*/, DWORD_PTR /*dwInstance*/,
                                DWORD /*fdwOpen*/)
    {
        if (!phwo || !pwfx) return MMSYSERR_ERROR;

        if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
        {
            if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return MMSYSERR_ERROR;
        }

        SDL_AudioSpec want{};
        want.freq = (int)pwfx->nSamplesPerSec;
        want.channels = (Uint8)pwfx->nChannels;
        want.format = wasm_pick_sdl_format(pwfx);
        want.samples = 1024;     // hint: tweak for latency/throughput
        want.callback = nullptr; // we push via SDL_QueueAudio

        WASM_WaveOut ctx{};
        ctx.dev = SDL_OpenAudioDevice(nullptr, 0, &want, &ctx.spec, 0);
        if (!ctx.dev) return MMSYSERR_ERROR;

        SDL_PauseAudioDevice(ctx.dev, 0); // start device

        // Allocate on heap for handle semantics
        WASM_WaveOut * heap = new WASM_WaveOut(ctx);
        *phwo = heap;
        return MMSYSERR_NOERROR;
    }

    inline MMRESULT waveOutClose(HWAVEOUT hwo)
    {
        if (!hwo) return MMSYSERR_NOERROR;
        if (hwo->dev) SDL_CloseAudioDevice(hwo->dev);
        delete hwo;
        return MMSYSERR_NOERROR;
    }

    inline MMRESULT waveOutPrepareHeader(HWAVEOUT /*hwo*/, LPWAVEHDR pwh, UINT /*cbwh*/)
    {
        if (!pwh) return MMSYSERR_ERROR;
        pwh->dwFlags |= WHDR_PREPARED;
        pwh->dwFlags &= ~(WHDR_DONE | WHDR_INQUEUE);
        return MMSYSERR_NOERROR;
    }

    inline MMRESULT waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT /*cbwh*/)
    {
        if (!hwo || !pwh || !pwh->lpData || pwh->dwBufferLength == 0) return MMSYSERR_ERROR;

        if (SDL_QueueAudio(hwo->dev, pwh->lpData, pwh->dwBufferLength) != 0) return MMSYSERR_ERROR;

        hwo->bytes_submitted += pwh->dwBufferLength;
        pwh->dwFlags &= ~WHDR_DONE;
        pwh->dwFlags |= (WHDR_PREPARED | WHDR_INQUEUE);

        // Track end offset to set WHDR_DONE later
        const uint64_t end_total = hwo->bytes_submitted;
        hwo->queue.push_back({ pwh, end_total });
        return MMSYSERR_NOERROR;
    }

    // Minimal MMTIME support: TIME_SAMPLES and TIME_MS
    inline MMRESULT waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT /*cbmmt*/)
    {
        if (!hwo || !pmmt) return MMSYSERR_ERROR;

        // Update WHDR_DONE flags for finished buffers
        hwo->pump_done_flags();

        const uint64_t played_bytes = hwo->bytes_played();
        const uint32_t fbytes = hwo->frame_bytes();
        const uint64_t frames = fbytes ? (played_bytes / fbytes) : 0;

        if (pmmt->wType == TIME_MS)
        {
            // ms = (frames * 1000) / sample_rate
            uint64_t ms = (hwo->spec.freq > 0) ? ((frames * 1000ull) / (uint64_t)hwo->spec.freq) : 0ull;
            pmmt->u.ms = (DWORD)ms;
            pmmt->wType = TIME_MS;
        }
        else
        {
            // default to TIME_SAMPLES
            pmmt->u.sample = (DWORD)frames;
            pmmt->wType = TIME_SAMPLES;
        }
        return MMSYSERR_NOERROR;
    }

    // waveOutReset: stop playback, mark all queued headers as DONE, clear queue,
    // reset logical position to zero (bytes_submitted = 0).
    inline MMRESULT waveOutReset(HWAVEOUT hwo)
    {
        if (!hwo) return MMSYSERR_ERROR;

        // Mark everything currently in our queue as DONE and not INQUEUE.
        for (auto & e : hwo->queue)
        {
            if (e.hdr)
            {
                e.hdr->dwFlags &= ~WHDR_INQUEUE;
                e.hdr->dwFlags |= WHDR_DONE;
                // Keep WHDR_PREPARED set (Windows keeps headers prepared after reset).
            }
        }
        hwo->queue.clear();

        // Drop any queued audio at SDL level and reset our position tracking.
        SDL_ClearQueuedAudio(hwo->dev);

        hwo->bytes_submitted = 0;

        // Device remains started; caller may re-queue new audio.
        return MMSYSERR_NOERROR;
    }

    // waveOutUnprepareHeader: if still queued => WAVERR_STILLPLAYING,
    // else clear WHDR_PREPARED (noop if not prepared).
    inline MMRESULT waveOutUnprepareHeader(HWAVEOUT /*hwo*/, LPWAVEHDR pwh, UINT /*cbwh*/)
    {
        if (!pwh) return MMSYSERR_ERROR;

        // If still queued and not yet done, unprepare is not allowed.
        if ((pwh->dwFlags & WHDR_INQUEUE) && !(pwh->dwFlags & WHDR_DONE))
        {
            return WAVERR_STILLPLAYING;
        }

        // If not prepared, Windows returns NOERROR (noop).
        if (!(pwh->dwFlags & WHDR_PREPARED))
        {
            return MMSYSERR_NOERROR;
        }

        // Clear PREPARED; leave DONE as-is (mirrors WinMM behavior).
        pwh->dwFlags &= ~WHDR_PREPARED;
        return MMSYSERR_NOERROR;
    }

#ifdef __EMSCRIPTEN__
    EM_ASYNC_JS(void, wait_next_raf, (), { await new Promise(requestAnimationFrame); });
#endif // __EMSCRIPTEN__

#endif // _WIN32

    void Update([[maybe_unused]] bool yield)
    {
#ifndef _WIN32
        if (!audioRunningFlag || hWave == nullptr) return;

        if (static bool primed = false; !primed)
        {
            WAVEHDR * wb = &waveBlocks[currBuffer];
            st3play::FillAudioBuffer((int16_t *)wb->lpData, MIX_BUF_SAMPLES);
            waveOutWrite(hWave, wb, sizeof(WAVEHDR));
            currBuffer = (currBuffer + 1) % MIX_BUF_NUM;
            g_totalSampleCount += MIX_BUF_SAMPLES;
            primed = true;
        }

        const uint32_t target_ms = 120;
        const uint32_t bytesPerFrame = waveBlocks[0].dwBufferLength / MIX_BUF_SAMPLES;
        const uint32_t targetBytes = (g_audioFreq * bytesPerFrame * target_ms) / 1000;

        MMTIME mm;
        mm.wType = TIME_SAMPLES;
        waveOutGetPosition(hWave, &mm, sizeof(mm));
        const uint32_t playedFrames = mm.u.sample;
        uint32_t queuedFrames = (g_totalSampleCount > playedFrames) ? (g_totalSampleCount - playedFrames) : 0;

        while (queuedFrames * bytesPerFrame < targetBytes)
        {
            WAVEHDR * wb = &waveBlocks[currBuffer];
            st3play::FillAudioBuffer((int16_t *)wb->lpData, MIX_BUF_SAMPLES);
            waveOutWrite(hWave, wb, sizeof(WAVEHDR));
            currBuffer = (currBuffer + 1) % MIX_BUF_NUM;
            queuedFrames += MIX_BUF_SAMPLES;
            g_totalSampleCount += MIX_BUF_SAMPLES;
        }
#endif //_WIN32

#ifdef __EMSCRIPTEN__
        static double lastPresent = 0.0;
        static double frameMS = 1000.0 / 60.0;
        static bool isFirefox = is_firefox();

        if (yield)
        {
            emscripten_sleep(0);
        }
        else if (double now = emscripten_get_now(); isFirefox && (now - lastPresent >= frameMS))
        {
            lastPresent = now;
            wait_next_raf();
        }
#endif // __EMSCRIPTEN__
    }

    bool openMixer(uint32_t audioFreq, int32_t & samplesLeft, uint32_t & totalSampleCount)
    {
        int32_t i;
        WAVEFORMATEX wfx;

        // don't unprepare headers on error
        for (i = 0; i < MIX_BUF_NUM; i++)
            waveBlocks[i].dwUser = 0xFFFF;

        closeMixer();

        memset(&wfx, 0, sizeof(wfx));
        wfx.nSamplesPerSec = audioFreq;
        wfx.wBitsPerSample = 16;
        wfx.nChannels = 2;
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        samplesLeft = 0;
        currBuffer = 0;
        totalSampleCount = 0;
        g_totalSampleCount = 0;

        if (waveOutOpen(&hWave, WAVE_MAPPER, &wfx, (DWORD_PTR)&waveProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) goto omError;

#ifdef _WIN32
        // create semaphore for buffer fill requests
        if (hAudioSem = CreateSemaphore(nullptr, MIX_BUF_NUM - 1, MIX_BUF_NUM, nullptr); hAudioSem == nullptr) goto omError;
#endif // _WIN32

        // allocate WinMM mix buffers
        for (i = 0; i < MIX_BUF_NUM; i++)
        {
            mixBuffer[i] = (int16_t *)calloc(MIX_BUF_SAMPLES, wfx.nBlockAlign);
            if (mixBuffer[i] == nullptr) goto omError;
        }

        // initialize WinMM mix headers
        memset(waveBlocks, 0, sizeof(waveBlocks));
        for (i = 0; i < MIX_BUF_NUM; i++)
        {
            waveBlocks[i].lpData = (LPSTR)mixBuffer[i];
            waveBlocks[i].dwBufferLength = MIX_BUF_SAMPLES * wfx.nBlockAlign;
            waveBlocks[i].dwFlags = WHDR_DONE;

            if (waveOutPrepareHeader(hWave, &waveBlocks[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) goto omError;
        }

        // create main mixer thread
        audioRunningFlag = true;

#ifdef _WIN32
        DWORD threadID;
        if (hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)mixThread, nullptr, 0, &threadID); hThread == nullptr) goto omError;
#endif // _WIN32

        return true;

    omError:
        closeMixer();
        return false;
    }

    void closeMixer()
    {
        int32_t i;

        audioRunningFlag = false; // make thread end when it's done

#ifdef _WIN32
        if (hAudioSem != nullptr) ReleaseSemaphore(hAudioSem, 1, nullptr);

        if (hThread != nullptr)
        {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            hThread = nullptr;
        }

        if (hAudioSem != nullptr)
        {
            CloseHandle(hAudioSem);
            hAudioSem = nullptr;
        }
#endif // _WIN32

        if (hWave != nullptr)
        {
            waveOutReset(hWave);
            for (i = 0; i < MIX_BUF_NUM; i++)
            {
                if (waveBlocks[i].dwUser != 0xFFFF) waveOutUnprepareHeader(hWave, &waveBlocks[i], sizeof(WAVEHDR));
            }

            waveOutClose(hWave);
            hWave = nullptr;
        }

        for (i = 0; i < MIX_BUF_NUM; i++)
        {
            if (mixBuffer[i] != nullptr)
            {
                free(mixBuffer[i]);
                mixBuffer[i] = nullptr;
            }
        }
    }

    uint32_t getSamplePosition()
    {
        MMTIME mmTime;
        mmTime.wType = TIME_SAMPLES;
        waveOutGetPosition(hWave, &mmTime, sizeof(MMTIME));

        return mmTime.u.sample;
    }

}