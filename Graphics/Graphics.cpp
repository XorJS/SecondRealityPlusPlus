#include "Graphics.h"

#ifdef _WIN32

#include <d3d11.h>
#pragma comment(lib, "d3d11")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

#include "Resources/resource.h"

#else

#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <SDL.h>
#include <SDL_opengles2.h>

#ifdef __EMSCRIPTEN__

#include <algorithm>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

extern "C"
{
    int is_firefox(void);
}

#endif // __EMSCRIPTEN__

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "Music/audioPlayer.h"

#endif // _WIN32

struct Float4
{
    float x{}, y{}, z{}, w{};
};

struct Float3
{
    float x{}, y{}, z{};
};

struct Float2
{
    float u{}, v{};
};

struct Vertex
{
    Float3 Pos{};
    Float2 Tex{};
};

struct CBChangesEveryFrame
{
    Float4 ratio{};
    Float4 JSSS{};
};

float g_width = 640.0f;
float g_height = 400.0f;
float g_sourceWidth = 1.0f;
float g_sourceHeight = 1.0f;
float g_jsss = 0.80f;

int g_demoScreenWidth = SCREEN_WIDTH;
int g_demoScreenHeight = SCREEN_HEIGHT;

unsigned int g_screen32[VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT]{};

#ifdef _DEBUG
const float ClearColor[4] = { 48.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f };
#else
const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
#endif // _DEBUG

bool g_wantsToQuit{};

#ifdef _WIN32

const char * shader = "Texture2D tex : register(t0); \
                       SamplerState samLinear : register(s0); \
                       SamplerState samPoint : register(s1); \
                       \
                       cbuffer cbChangesEveryFrame : register(b0) \
                       { \
                           float4 ratio; \
                           float4 JSSS; \
                       }; \
                       \
                       struct VS_INPUT \
                       { \
                           float4 Pos : POSITION; \
                           float2 Tex : TEXCOORD0; \
                       }; \
                       \
                       struct PS_INPUT \
                       { \
                           float4 Pos : SV_POSITION; \
                           float2 Tex : TEXCOORD0; \
                       }; \
                       \
                       PS_INPUT VS(VS_INPUT input) \
                       { \
                           PS_INPUT output = (PS_INPUT)0; \
                           float2 pos = 2.0f * input.Tex - 1.0f; \
                           output.Pos = float4(pos * ratio.xy, 0.0f, 1.0f); \
                           output.Tex = ratio.zw * abs(float2(0.0f, 1.0f) - input.Tex); \
                           return output; \
                       } \
                       \
                       float4 PS(PS_INPUT input) : SV_Target \
                       { \
                           float4 jsSuperSampling = lerp(tex.Sample(samLinear, input.Tex), tex.Sample(samPoint, input.Tex), JSSS.x); \
                           return jsSuperSampling; \
                       } ";

HINSTANCE g_hInst = nullptr;
HWND g_hWnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device * g_pd3dDevice = nullptr;
ID3D11DeviceContext * g_pImmediateContext = nullptr;
IDXGISwapChain * g_pSwapChain = nullptr;
ID3D11RenderTargetView * g_pRenderTargetView = nullptr;
ID3D11VertexShader * g_pVertexShader = nullptr;
ID3D11PixelShader * g_pPixelShader = nullptr;
ID3D11InputLayout * g_pVertexLayout = nullptr;
ID3D11Buffer * g_pVertexBuffer = nullptr;
ID3D11Buffer * g_pIndexBuffer = nullptr;
ID3D11Buffer * g_pCBChangesEveryFrame = nullptr;

ID3D11Texture2D * g_texture = nullptr;
ID3D11ShaderResourceView * g_textureRV = nullptr;

ID3D11SamplerState * g_pSamplerLinear = nullptr;
ID3D11SamplerState * g_pSamplerPoint = nullptr;

template <typename T>
void SafeRelease(T & buffer)
{
    if (buffer != nullptr)
    {
        buffer->Release();
        buffer = nullptr;
    }
}

HRESULT CreateRenderTarget()
{
    ID3D11Texture2D * pBackBuffer = nullptr;

    if (HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&pBackBuffer); FAILED(hr))
    {
        return hr;
    }

    HRESULT hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);

    SafeRelease(pBackBuffer);

    return hr;
}

void CleanupRenderTarget()
{
    SafeRelease(g_pRenderTargetView);
}

HRESULT CompileShaderFromMemory(const char * text, LPCSTR entryPoint, LPCSTR target, ID3DBlob ** shaderBlob)
{
    ID3DBlob * errorBlob = nullptr;

    DWORD compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    if (HRESULT hr = D3DCompile(text, strlen(text), nullptr, nullptr, nullptr, entryPoint, target, compileFlags, 0, shaderBlob, &errorBlob); FAILED(hr))
    {
        if (errorBlob != nullptr)
        {
            MessageBoxA(nullptr, (char *)errorBlob->GetBufferPointer(), "Error", MB_OK);
        }

        SafeRelease(errorBlob);

        return hr;
    }

    SafeRelease(errorBlob);

    return S_OK;
}

HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    DXGI_SWAP_CHAIN_DESC swapchainDescriptor{};
    swapchainDescriptor.BufferCount = 1;
    swapchainDescriptor.BufferDesc.Width = width;
    swapchainDescriptor.BufferDesc.Height = height;
    swapchainDescriptor.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDescriptor.BufferDesc.RefreshRate.Numerator = 60;
    swapchainDescriptor.BufferDesc.RefreshRate.Denominator = 1;
    swapchainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDescriptor.OutputWindow = g_hWnd;
    swapchainDescriptor.SampleDesc.Count = 1;
    swapchainDescriptor.SampleDesc.Quality = 0;
    swapchainDescriptor.Windowed = true;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];

        hr = D3D11CreateDeviceAndSwapChain(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
                                           &swapchainDescriptor, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

        if (SUCCEEDED(hr)) break;
    }

    if (FAILED(hr)) return hr;

    if (FAILED(CreateRenderTarget())) return hr;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    D3D11_VIEWPORT viewport;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &viewport);

    ID3DBlob * pVSBlob = nullptr;
    if (hr = CompileShaderFromMemory(shader, "VS", "vs_4_0", &pVSBlob); FAILED(hr)) return hr;

    if (hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader); FAILED(hr))
    {
        SafeRelease(pVSBlob);
        return hr;
    }

    const D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(inputLayout);

    hr = g_pd3dDevice->CreateInputLayout(inputLayout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);

    SafeRelease(pVSBlob);

    if (FAILED(hr)) return hr;

    g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

    ID3DBlob * pPSBlob = nullptr;
    if (hr = CompileShaderFromMemory(shader, "PS", "ps_4_0", &pPSBlob); FAILED(hr)) return hr;

    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    SafeRelease(pPSBlob);
    if (FAILED(hr)) return hr;

    const Vertex vertices[] = {
        { Float3(-1.0f, 1.0f, -1.0f), Float2(0.0f, 0.0f) },
        { Float3(1.0f, 1.0f, -1.0f), Float2(1.0f, 0.0f) },
        { Float3(1.0f, 1.0f, 1.0f), Float2(1.0f, 1.0f) },
        { Float3(-1.0f, 1.0f, 1.0f), Float2(0.0f, 1.0f) },
    };

    D3D11_BUFFER_DESC bufferDescriptor{};
    bufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    bufferDescriptor.ByteWidth = sizeof(vertices);
    bufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDescriptor.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = vertices;
    if (hr = g_pd3dDevice->CreateBuffer(&bufferDescriptor, &initData, &g_pVertexBuffer); FAILED(hr)) return hr;

    UINT stride = sizeof(vertices[0]);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    const WORD indices[] = {
        3, 1, 0, 2, 1, 3,
    };

    bufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    bufferDescriptor.ByteWidth = sizeof(indices);
    bufferDescriptor.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDescriptor.CPUAccessFlags = 0;
    initData.pSysMem = indices;
    if (hr = g_pd3dDevice->CreateBuffer(&bufferDescriptor, &initData, &g_pIndexBuffer); FAILED(hr)) return hr;

    g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    bufferDescriptor.Usage = D3D11_USAGE_DEFAULT;
    bufferDescriptor.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDescriptor.CPUAccessFlags = 0;

    bufferDescriptor.ByteWidth = sizeof(CBChangesEveryFrame);
    if (hr = g_pd3dDevice->CreateBuffer(&bufferDescriptor, nullptr, &g_pCBChangesEveryFrame); FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC textureDescriptor = {};
    textureDescriptor.Width = VIRTUAL_SCREEN_WIDTH;
    textureDescriptor.Height = VIRTUAL_SCREEN_HEIGHT;
    textureDescriptor.MipLevels = 1;
    textureDescriptor.ArraySize = 1;
    textureDescriptor.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDescriptor.SampleDesc = { 1, 0 };
    textureDescriptor.Usage = D3D11_USAGE_DYNAMIC;
    textureDescriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (hr = g_pd3dDevice->CreateTexture2D(&textureDescriptor, nullptr, &g_texture); FAILED(hr)) return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDescriptor = {};
    shaderResourceViewDescriptor.Format = textureDescriptor.Format;
    shaderResourceViewDescriptor.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDescriptor.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDescriptor.Texture2D.MipLevels = 1;
    if (hr = g_pd3dDevice->CreateShaderResourceView(g_texture, &shaderResourceViewDescriptor, &g_textureRV); FAILED(hr)) return hr;

    D3D11_SAMPLER_DESC sampleDescriptor{};
    sampleDescriptor.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampleDescriptor.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampleDescriptor.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampleDescriptor.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampleDescriptor.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampleDescriptor.MinLOD = 0;
    sampleDescriptor.MaxLOD = D3D11_FLOAT32_MAX;

    if (hr = g_pd3dDevice->CreateSamplerState(&sampleDescriptor, &g_pSamplerLinear); FAILED(hr)) return hr;

    sampleDescriptor.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

    if (hr = g_pd3dDevice->CreateSamplerState(&sampleDescriptor, &g_pSamplerPoint); FAILED(hr)) return hr;

    return S_OK;
}

void CleanupDevice()
{
    if (g_pImmediateContext) g_pImmediateContext->ClearState();

    SafeRelease(g_pSamplerLinear);
    SafeRelease(g_pSamplerPoint);
    SafeRelease(g_texture);
    SafeRelease(g_textureRV);
    SafeRelease(g_pCBChangesEveryFrame);
    SafeRelease(g_pVertexBuffer);
    SafeRelease(g_pIndexBuffer);
    SafeRelease(g_pVertexLayout);
    SafeRelease(g_pVertexShader);
    SafeRelease(g_pPixelShader);

    CleanupRenderTarget();

    SafeRelease(g_pSwapChain);
    SafeRelease(g_pImmediateContext);
    SafeRelease(g_pd3dDevice);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE: {
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
            {
                CleanupRenderTarget();

                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

                CreateRenderTarget();
            }

            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);

            break;
        }

        case WM_KEYDOWN: {
            switch (wParam)
            {
                case VK_ESCAPE: {
                    g_wantsToQuit = true;
                    PostMessage(hWnd, WM_QUIT, 0, 0);
                    return 0L;
                }
            }

            break;
        }

        case WM_CLOSE:
        case WM_DESTROY: {
            g_wantsToQuit = true;
            PostMessage(hWnd, WM_QUIT, 0, 0);

            break;
        }

        case WM_SYSCOMMAND: {
            switch (wParam)
            {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER: {
                    return 0;
                }
            }

            break;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Render()
{
    DXGI_SWAP_CHAIN_DESC swapChainDescriptor;
    g_pSwapChain->GetDesc(&swapChainDescriptor);

    g_width = static_cast<float>(swapChainDescriptor.BufferDesc.Width);
    g_height = static_cast<float>(swapChainDescriptor.BufferDesc.Height);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    D3D11_VIEWPORT vp;
    vp.Width = static_cast<FLOAT>(g_width);
    vp.Height = static_cast<FLOAT>(g_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports(1, &vp);

    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

    CBChangesEveryFrame cb;

    int ratioX = static_cast<int>(g_width / SCREEN_WIDTH);
    int ratioY = static_cast<int>(g_height / SCREEN_HEIGHT);

    float ratio = max(1.0f, static_cast<float>(min(ratioX, ratioY)));

    cb.ratio.x = ratio * (SCREEN_WIDTH / g_width);
    cb.ratio.y = ratio * (SCREEN_HEIGHT / g_height);
    cb.ratio.z = g_sourceWidth;
    cb.ratio.w = g_sourceHeight;

    cb.JSSS.x = g_jsss;

    g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

    g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);

    g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBChangesEveryFrame);
    g_pImmediateContext->PSSetShaderResources(0, 1, &g_textureRV);
    g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
    g_pImmediateContext->PSSetSamplers(1, 1, &g_pSamplerPoint);
    g_pImmediateContext->DrawIndexed(6, 0, 0);

    g_pSwapChain->Present(0, 0);
}

bool Graphics::Init(WindowType _windowType)
{
    if (HWND consoleWindowHandle = GetConsoleWindow(); consoleWindowHandle != nullptr)
    {
        ShowWindow(consoleWindowHandle, SW_HIDE);
    }

    SetProcessDPIAware();

    HINSTANCE instance = GetModuleHandle(nullptr);

    WNDCLASSEX wndClass{};
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = instance;
    wndClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = nullptr;
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = L"SecondReality++";

    if (!RegisterClassEx(&wndClass))
    {
        return false;
    }

    RECT windowRect = { 0, 0, 1280, 720 };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    g_hWnd = CreateWindow(wndClass.lpszClassName, L"Second Reality++",
                          (_windowType == WindowType::Windowed) ? WS_OVERLAPPEDWINDOW : WS_POPUP | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                          windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, instance, nullptr);

    if (!g_hWnd)
    {
        return false;
    }

    ShowWindow(g_hWnd, SW_MAXIMIZE);
    SetForegroundWindow(g_hWnd);
    SetFocus(g_hWnd);

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    ShowCursor(_windowType == WindowType::Windowed);

    g_wantsToQuit = false;

    return 1;
}

void Graphics::Close()
{
    CleanupDevice();

    DestroyWindow(g_hWnd);
}

void Graphics::HandleMessages()
{
    if (MSG msg; PeekMessage(&msg, g_hWnd, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Graphics::Update()
{
    HandleMessages();

    PrepareTextureForGPU();

    ID3D11ShaderResourceView * nullShaderResourceView[1] = { nullptr };
    g_pImmediateContext->PSSetShaderResources(0, 1, nullShaderResourceView);

    D3D11_MAPPED_SUBRESOURCE mapSubResource{};

    if (SUCCEEDED(g_pImmediateContext->Map(g_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapSubResource)))
    {
        size_t srcStrideBytes = VIRTUAL_SCREEN_WIDTH * 4;

        const uint8_t * src = reinterpret_cast<const uint8_t *>(g_screen32);
        uint8_t * dst = static_cast<uint8_t *>(mapSubResource.pData);

        const size_t rowCopy = (srcStrideBytes < mapSubResource.RowPitch) ? srcStrideBytes : mapSubResource.RowPitch;

        for (auto y = 0; y < VIRTUAL_SCREEN_HEIGHT; ++y)
        {
            memcpy(dst, src, rowCopy);

            src += srcStrideBytes;
            dst += mapSubResource.RowPitch;
        }

        g_pImmediateContext->Unmap(g_texture, 0);
    }

    Render();
}

#else

namespace
{
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_WebGL_context = 0;
#endif

    SDL_Window * g_windows = nullptr;
    SDL_GLContext g_context = nullptr;

    GLuint g_prog = 0;
    GLint g_Loc_uResolution = -1;
    GLint g_Loc_uTex0 = -1;
    GLint g_Loc_uTex1 = -1;
    GLuint g_VBO = 0;
    GLuint g_texture = 0;
    GLint g_Loc_aPos = -1;
    GLint g_Loc_aUV = -1;
    GLuint g_linearSampling = 0;
    GLuint g_nearestSampling = 0;
}

static void die(const char * msg)
{
    std::fprintf(stderr, "FATAL: %s\n", msg);
    std::exit(1);
}

static GLuint compileShader(GLenum type, const char * src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error:\n%s\n", log.c_str());
        die("compileShader failed");
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);

    glBindAttribLocation(p, 0, "aPos");
    glBindAttribLocation(p, 1, "aUV");

    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link error:\n%s\n", log.c_str());
        die("linkProgram failed");
    }

    return p;
}

GLuint makeTextureRGBA8(int w, int h)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    return tex;
}

void UpdateTextureRGBA8(GLuint tex, int w, int h, const void * pixels)
{
    glBindTexture(GL_TEXTURE_2D, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void initGL()
{
    const char * vertexShader = "precision mediump float;\n"
                                "attribute vec2 aPos;\n"
                                "attribute vec2 aUV;\n"
                                "uniform   vec3 uResolution;\n"
                                "varying   vec2 vUV;\n"
                                "void main(){\n"
                                "  vec2 pos = (aPos / uResolution.xy) * 2.0 - 1.0;\n"
                                "  pos.y = -pos.y;\n"
                                "  gl_Position = vec4(pos, 0.0, 1.0);\n"
                                "  vUV = aUV;\n"
                                "}\n";

    const char * fragmentShader = "precision mediump float;\n"
                                  "varying vec2 vUV;\n"
                                  "uniform vec3 uResolution;\n"
                                  "uniform sampler2D uTex0;\n"
                                  "uniform sampler2D uTex1;\n"
                                  "void main(){\n"
                                  "  gl_FragColor = vec4(mix(texture2D(uTex0, vUV).bgr, texture2D(uTex1, vUV).bgr, uResolution.z),1);\n"
                                  "}\n";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    g_prog = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_Loc_uResolution = glGetUniformLocation(g_prog, "uResolution");
    g_Loc_uTex0 = glGetUniformLocation(g_prog, "uTex0");
    g_Loc_uTex1 = glGetUniformLocation(g_prog, "uTex1");
    g_Loc_aPos = 0;
    g_Loc_aUV = 1;

    glGenBuffers(1, &g_VBO);

    g_texture = makeTextureRGBA8(VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);

    glGenSamplers(1, &g_linearSampling);
    glGenSamplers(1, &g_nearestSampling);

    glSamplerParameteri(g_linearSampling, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(g_linearSampling, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glSamplerParameteri(g_linearSampling, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_linearSampling, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUseProgram(g_prog);
    glUniform1i(g_Loc_uTex0, 0);
    glUniform1i(g_Loc_uTex1, 1);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
}

static void Render()
{
    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);

    glViewport(0, 0, g_width, g_height);

    const int ratioX = static_cast<int>(g_width / SCREEN_WIDTH);
    const int ratioY = static_cast<int>(g_height / SCREEN_HEIGHT);

    const float ratio = fmax(1.0f, static_cast<float>(fmin(ratioX, ratioY)));

    const float w = ratio * SCREEN_WIDTH;
    const float h = ratio * SCREEN_HEIGHT;

    const float x = (g_width - w) * 0.5f;
    const float y = (g_height - h) * 0.5f;

    const GLfloat verts[] = {
        x,     y,     0.0f,          0.0f,           // top-left
        x,     y + h, 0.0f,          g_sourceHeight, // bottom-left
        x + w, y,     g_sourceWidth, 0.0f,           // top-right
        x + w, y + h, g_sourceWidth, g_sourceHeight  // bottom-right
    };

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g_prog);

    glUniform3f(g_Loc_uResolution, g_width, g_height, g_jsss);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glBindSampler(0, g_linearSampling);
    glBindSampler(1, g_nearestSampling);

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glEnableVertexAttribArray(g_Loc_aPos);
    glEnableVertexAttribArray(g_Loc_aUV);
    glVertexAttribPointer(g_Loc_aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)(0));
    glVertexAttribPointer(g_Loc_aUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(g_Loc_aPos);
    glDisableVertexAttribArray(g_Loc_aUV);
    glUseProgram(0);

    SDL_GL_SwapWindow(g_windows);
}

#ifdef __EMSCRIPTEN__
static void SyncCanvasToCSSAndViewport()
{
    double cssW = 0.0, cssH = 0.0;
    emscripten_get_element_css_size("#canvas", &cssW, &cssH);

    const double dpr = emscripten_get_device_pixel_ratio();
    const int width = (int)std::lround(cssW * dpr);
    const int height = (int)std::lround(cssH * dpr);

    emscripten_set_canvas_element_size("#canvas", width, height);

    if (g_WebGL_context)
    {
        emscripten_webgl_make_context_current(g_WebGL_context);
    }

    glViewport(0, 0, width, height);

    g_width = static_cast<float>(width);
    g_height = static_cast<float>(height);
}

static EM_BOOL OnResize(int, const EmscriptenUiEvent *, void *)
{
    SyncCanvasToCSSAndViewport();

    emscripten_set_timeout([](void *) { SyncCanvasToCSSAndViewport(); }, 16, nullptr);

    return EM_TRUE;
}

void InitSizesAndLlisteners()
{
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.antialias = false;
    attrs.enableExtensionsByDefault = 1;
    g_WebGL_context = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(g_WebGL_context);

    SyncCanvasToCSSAndViewport();
    emscripten_set_timeout([](void *) { SyncCanvasToCSSAndViewport(); }, 0, nullptr);

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnResize);
}

// clang-format off
EM_JS(int, is_macos, (), {
    const nav = navigator || {};
    // Prefer UA-CH if present
    if (nav.userAgentData && nav.userAgentData.platform) {
      const isMac = nav.userAgentData.platform === 'macOS';
      // iPadOS may not expose UA-CH; fall through if so.
      if (isMac) return 1;
    }
    const ua = (nav.userAgent || ' ');
    const plt = (nav.platform || ' ');
    const looksMac = /Macintosh|Mac OS X/.test(ua) || /^Mac/.test(plt);
    const isIPadOS = (plt === 'MacIntel' && (nav.maxTouchPoints || 0) > 1);
    return (looksMac && !isIPadOS) ? 1 : 0;
});

EM_JS(void, Wake_StartFallbackVideo, (), {
    if (window.__wlFallback) return;
    try
    {
        const c = document.createElement('canvas');
        c.width = 2;
        c.height = 2;
        const cx = c.getContext('2d');
        cx.fillRect(0, 0, 2, 2);
        let on = false;
        (function tick() {
            on = !on;
            cx.fillStyle = on ? '#001' : '#000';
            cx.fillRect(0, 0, 2, 2);
            setTimeout(() => requestAnimationFrame(tick), 1000);
        })();
        const stream = c.captureStream(1);
        const v = document.createElement('video');
        v.playsInline = true;
        v.muted = true;
        v.autoplay = true;
        v.loop = true;
        v.srcObject = stream;
        v.style.cssText = 'position:fixed;width:1px;height:1px;opacity:0;left:0;top:0;pointer-events:none';
        document.body.appendChild(v);
        v.play().catch(() => {});
        window.__wlFallback = { v, stream };
        console.log('[wake] fallback video started');
    }
    catch (e)
    {
        window.__wl_lastErr = 'fallback:' + (e && e.name || 'err');
    }
});

EM_JS(void, Wake_TryNow_NoAwait, (), {
    if (!('wakeLock' in navigator))
    {
        window.__wl_lastErr = 'unsupported';
        return;
    }
    if (window.__wl && !window.__wl.released) return;

    navigator.wakeLock.request('screen')
        .then(s =>
                  {
                      window.__wl = s;
                      window.__wl_lastErr = ' ';
                      console.log('[wake] acquired');
                      s.addEventListener(
                          'release', () => {
                              console.log('[wake] released');
                              window.__wl = null;
                          });
                  })
        .catch(e => {
            window.__wl_lastErr = (e && e.name) || 'denied';
            console.warn('[wake] denied:', window.__wl_lastErr);
        });
});

EM_JS(void, Wake_InstallLifecycle, (), {
    if (Module.__wl_life) return;
    Module.__wl_life = true;
    const retry = () =>
    {
        if (!window.__wl || window.__wl.released)
        {
            if ('wakeLock' in navigator && document.visibilityState === 'visible')
            {
                navigator.wakeLock.request('screen')
                    .then(s =>
                              {
                                  window.__wl = s;
                                  window.__wl_lastErr = ' ';
                              })
                    .catch(e => { window.__wl_lastErr = (e && e.name) || 'denied'; });
            }
        }
    };
    document.addEventListener('visibilitychange', retry, true);
    window.addEventListener('pageshow', retry, true);
    window.addEventListener('focus', retry, true);
    if (screen.orientation && screen.orientation.addEventListener) screen.orientation.addEventListener('change', retry, true);
});

EM_JS(void, Wake_OnGesture_Begin, (), {
    Wake_TryNow_NoAwait();
    if (!window.__wl || window.__wl.released)
    {
        if (window.__wl_lastErr)
        {
            Wake_StartFallbackVideo();
        }
    }
});

EM_JS(void, Wake_AfterFullscreenChange, (), {
    // Some WebKit builds only allow lock after fullscreen transition.
    if (!window.__wl || window.__wl.released)
    {
        Wake_TryNow_NoAwait();
        if (!window.__wl || window.__wl.released)
        {
            if (window.__wl_lastErr)
            {
                Wake_StartFallbackVideo();
            }
        }
    }
});
// clang-format on

void ToggleFullScreen()
{
    if (static bool isMacOS = is_macos(); isMacOS)
    {
        return;
    }

    EmscriptenFullscreenChangeEvent st;

    if (emscripten_get_fullscreen_status(&st); st.isFullscreen)
    {
        emscripten_exit_fullscreen();
    }
    else
    {
        emscripten_request_fullscreen("#canvas", EM_FALSE);
    }
}

EM_BOOL OnMouseDown(int, const EmscriptenMouseEvent *, void *)
{
    Wake_OnGesture_Begin();

    ToggleFullScreen();

    return EM_TRUE;
}

EM_BOOL OnTouchStart(int, const EmscriptenTouchEvent *, void *)
{
    Wake_OnGesture_Begin();

    ToggleFullScreen();

    return EM_TRUE;
}

void InstallClickAnywhereFullscreen()
{
    emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, OnMouseDown);
    emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, OnTouchStart);
    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_FALSE,
                                             [](int, const EmscriptenFullscreenChangeEvent *, void *) -> EM_BOOL {
                                                 SyncCanvasToCSSAndViewport();
                                                 Wake_AfterFullscreenChange();
                                                 return EM_TRUE;
                                             });
    Wake_InstallLifecycle();
}

#else

bool is_macos()
{
    return false;
}
bool is_firefox()
{
    return false;
}
void InitSizesAndLlisteners() {}
void InstallClickAnywhereFullscreen() {}

#endif // __EMSCRIPTEN__

bool Graphics::Init(WindowType _fullscreen)
{
    SDL_DisableScreenSaver();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    InitSizesAndLlisteners();
    InstallClickAnywhereFullscreen();

    g_windows =
        SDL_CreateWindow("Second Reality++", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_width, g_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!g_windows)
    {
        die("SDL_CreateWindow failed");
    }

    if (g_context = SDL_GL_CreateContext(g_windows); !g_context)
    {
        die("SDL_GL_CreateContext failed");
    }

    SDL_GL_MakeCurrent(g_windows, g_context);
    SDL_GL_SetSwapInterval(0);

    SDL_MaximizeWindow(g_windows);

    if (_fullscreen == WindowType::Fullscreen)
    {
        SDL_SetWindowFullscreen(g_windows, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    }

    initGL();

    return true;
}

void Graphics::Update()
{
#ifndef __EMSCRIPTEN__
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        if ((ev.type == SDL_QUIT) || (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE))
        {
            g_wantsToQuit = true;
        }

        if (ev.type == SDL_WINDOWEVENT)
        {
            if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                g_width = ev.window.data1;
                g_height = ev.window.data2;
            }
        }
    }
#endif // __EMSCRIPTEN__

    PrepareTextureForGPU();

    UpdateTextureRGBA8(g_texture, VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT, g_screen32);

    Render();

    static bool isFirefox = is_firefox();
    AudioPlayer::Update(!isFirefox);
}

void Graphics::Close()
{
#ifndef __EMSCRIPTEN__

    if (g_linearSampling) glDeleteSamplers(1, &g_linearSampling);
    if (g_nearestSampling) glDeleteSamplers(1, &g_nearestSampling);
    if (g_texture) glDeleteTextures(1, &g_texture);
    if (g_VBO) glDeleteBuffers(1, &g_VBO);
    if (g_prog) glDeleteProgram(g_prog);

    SDL_GL_DeleteContext(g_context);
    SDL_DestroyWindow(g_windows);
    SDL_Quit();
#endif // __EMSCRIPTEN__

    SDL_EnableScreenSaver();
}

#endif // _WIN32

bool Graphics::WantsToQuit()
{
    return g_wantsToQuit;
}

void Graphics::PrepareTextureForGPU()
{
    if (g_demoScreenWidth == SCREEN_WIDTH && g_demoScreenHeight == SCREEN_HEIGHT)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                unsigned int c = Shim::palette[*src++];
                *dst++ = c;
            }

            dst += SCREEN_WIDTH;
        }
    }
    else if (g_demoScreenWidth == SCREEN_WIDTH && g_demoScreenHeight == DOUBlE_SCREEN_HEIGHT)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                unsigned int c = Shim::palette[*src++];
                *dst++ = c;
            }

            dst += SCREEN_WIDTH;
        }
    }
    else if (g_demoScreenWidth == 640 && g_demoScreenHeight == 350)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32 + VIRTUAL_SCREEN_WIDTH * (VIRTUAL_SCREEN_HEIGHT - 350) / 2;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                *dst++ = Shim::palette[*src++];
            }
        }
    }

    g_sourceWidth = static_cast<float>(g_demoScreenWidth) / VIRTUAL_SCREEN_WIDTH;
    g_sourceHeight = static_cast<float>(g_demoScreenHeight) / VIRTUAL_SCREEN_HEIGHT;
}

void Graphics::ChangeMode(int x, int y, float jsss)
{
    g_demoScreenWidth = x;
    g_demoScreenHeight = y;

    g_jsss = jsss;

    Common::reset();

    CLEAR(g_screen32);
}
