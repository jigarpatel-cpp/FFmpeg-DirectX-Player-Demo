// File: src/dx11_renderer.cpp
#include "dx11_renderer.h"
#include <windowsx.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DX11Renderer::DX11Renderer() {}
DX11Renderer::~DX11Renderer() { cleanup(); }

bool DX11Renderer::createWindow()
{
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"FFmpegPlayerClass";

    if (!RegisterClassW(&wc)) return false;

    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"FFmpeg DirectX11 Player",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd) return false;
    ShowWindow(hwnd, SW_SHOW);
    return true;
}

bool DX11Renderer::initialize(int w, int h) {
    width = w;
    height = h;
    if (!createWindow()) return false;

    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &swapChain,
        &device,
        nullptr,
        &context
    );

    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to create DirectX device and swap chain", "Error", MB_OK);
        return false;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to get back buffer", "Error", MB_OK);
        return false;
    }

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
    backBuffer->Release();
    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to create render target view", "Error", MB_OK);
        return false;
    }

    context->OMSetRenderTargets(1, &rtv, nullptr);

    D3D11_VIEWPORT viewport = { 0, 0, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    context->RSSetViewports(1, &viewport);

    if (!createShaders() || !createBuffers()) {
        return false;
    }

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &samplerState);
    if (FAILED(hr)) {
        MessageBoxA(0, "Failed to create sampler state", "Error", MB_OK);
        return false;
    }

    return true;
}

void DX11Renderer::updateTexture(AVFrame* frame) {
    if (!texture) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        device->CreateTexture2D(&desc, nullptr, &texture);
        device->CreateShaderResourceView(texture, nullptr, &srv);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        for (int y = 0; y < height; ++y)
            memcpy((BYTE*)mapped.pData + y * mapped.RowPitch, frame->data[0] + y * frame->linesize[0], width * 4);
        context->Unmap(texture, 0);
    }
}

void DX11Renderer::renderTexture() {
    if (!srv || !vertexBuffer || !vertexShader || !pixelShader || !inputLayout || !samplerState)
        return;

    UINT stride = sizeof(XMFLOAT4) + sizeof(XMFLOAT2);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->IASetInputLayout(inputLayout);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &srv);
    context->PSSetSamplers(0, 1, &samplerState);

    context->Draw(4, 0);
}

void DX11Renderer::renderFrame(AVFrame* frame) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            cleanup();
            exit(0);
        }
    }

    updateTexture(frame);

    FLOAT bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(rtv, bgColor);
    
    renderTexture();
    
    swapChain->Present(1, 0);
}

bool DX11Renderer::createShaders() {
    const char* vertexShaderCode = 
        "struct VS_INPUT {"
        "    float4 pos : POSITION;"
        "    float2 tex : TEXCOORD;"
        "};"
        "struct PS_INPUT {"
        "    float4 pos : SV_POSITION;"
        "    float2 tex : TEXCOORD;"
        "};"
        "PS_INPUT main(VS_INPUT input) {"
        "    PS_INPUT output;"
        "    output.pos = input.pos;"
        "    output.tex = input.tex;"
        "    return output;"
        "}";

    const char* pixelShaderCode =
        "Texture2D tex : register(t0);"
        "SamplerState sampler0 : register(s0);"
        "struct PS_INPUT {"
        "    float4 pos : SV_POSITION;"
        "    float2 tex : TEXCOORD;"
        "};"
        "float4 main(PS_INPUT input) : SV_Target {"
        "    return tex.Sample(sampler0, input.tex);"
        "}";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), nullptr, nullptr, nullptr,
        "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(0, (char*)errorBlob->GetBufferPointer(), "Vertex Shader Error", MB_OK);
            errorBlob->Release();
        }
        return false;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        vsBlob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return false;

    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), nullptr, nullptr, nullptr,
        "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            MessageBoxA(0, (char*)errorBlob->GetBufferPointer(), "Pixel Shader Error", MB_OK);
            errorBlob->Release();
        }
        return false;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    psBlob->Release();
    if (FAILED(hr)) return false;

    return true;
}

bool DX11Renderer::createBuffers() {
    struct Vertex {
        XMFLOAT4 pos;
        XMFLOAT2 tex;
    };

    Vertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&bd, &initData, &vertexBuffer);
    if (FAILED(hr)) return false;

    return true;
}

void DX11Renderer::cleanup() {
    if (samplerState) samplerState->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (inputLayout) inputLayout->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (srv) srv->Release();
    if (texture) texture->Release();
    if (rtv) rtv->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    if (hwnd) DestroyWindow(hwnd);

    samplerState = nullptr;
    vertexBuffer = nullptr;
    inputLayout = nullptr;
    vertexShader = nullptr;
    pixelShader = nullptr;
    srv = nullptr;
    texture = nullptr;
    rtv = nullptr;
    swapChain = nullptr;
    context = nullptr;
    device = nullptr;
    hwnd = nullptr;
}
