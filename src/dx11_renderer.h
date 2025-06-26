// File: src/dx11_renderer.h
#pragma once
#include <d3d11.h>
#include <libavutil/frame.h>

class DX11Renderer {
public:
    DX11Renderer();
    ~DX11Renderer();

    bool initialize(int width, int height);
    void renderFrame(AVFrame* frame);
    void cleanup();

private:
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11SamplerState* samplerState = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11Buffer* vertexBuffer = nullptr;

    bool createWindow();
    bool createShaders();
    bool createBuffers();
    void updateTexture(AVFrame* frame);
    void renderTexture();
};
