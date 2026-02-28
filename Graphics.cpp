#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h> 
#include <assert.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib") 

ID3D11Device* m_pDevice = nullptr;
ID3D11DeviceContext* m_pDeviceContext = nullptr;
IDXGISwapChain* m_pSwapChain = nullptr;
ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;

ID3D11Buffer* m_pVertexBuffer = nullptr;
ID3D11Buffer* m_pIndexBuffer = nullptr;
ID3D11InputLayout* m_pInputLayout = nullptr;
ID3D11VertexShader* m_pVertexShader = nullptr;
ID3D11PixelShader* m_pPixelShader = nullptr;

int m_width = 1280;
int m_height = 720;

struct Vertex {
    float x, y, z;
    COLORREF color;
};

const char* shaderCode =
"struct VSInput { float3 pos : POSITION; float4 color : COLOR; };\n"
"struct VSOutput { float4 pos : SV_Position; float4 color : COLOR; };\n"
"VSOutput vs(VSInput vertex) {\n"
"    VSOutput result;\n"
"    result.pos = float4(vertex.pos, 1.0);\n"
"    result.color = vertex.color;\n"
"    return result;\n"
"}\n"
"float4 ps(VSOutput pixel) : SV_Target0 {\n"
"    return pixel.color;\n"
"}";

HRESULT CreateBackBuffer() {
    ID3D11Texture2D* pBackBuffer = NULL;
    HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (SUCCEEDED(result))
    {
        result = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pBackBufferRTV);
        pBackBuffer->Release();
    }
    return result;
}

HRESULT InitScene() {
    HRESULT result;

    static const Vertex Vertices[] = {
        {-0.5f, -0.5f, 0.0f, RGB(255, 0, 0)},
        { 0.5f, -0.5f, 0.0f, RGB(0, 255, 0)},
        { 0.0f,  0.5f, 0.0f, RGB(0, 0, 255)}
    };
    static const USHORT Indices[] = { 0, 2, 1 };

    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.ByteWidth = sizeof(Vertices);
    vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexData = { Vertices };
    result = m_pDevice->CreateBuffer(&vertexDesc, &vertexData, &m_pVertexBuffer);
    if (FAILED(result)) return result;

    D3D11_BUFFER_DESC indexDesc = {};
    indexDesc.ByteWidth = sizeof(Indices);
    indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexData = { Indices };
    result = m_pDevice->CreateBuffer(&indexDesc, &indexData, &m_pIndexBuffer);
    if (FAILED(result)) return result;

    ID3DBlob* pVSBlob = nullptr;
    result = D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "vs", "vs_5_0", 0, 0, &pVSBlob, NULL);
    if (SUCCEEDED(result)) {
        result = m_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &m_pVertexShader);
    }

    ID3DBlob* pPSBlob = nullptr;
    result = D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "ps", "ps_5_0", 0, 0, &pPSBlob, NULL);
    if (SUCCEEDED(result)) {
        result = m_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &m_pPixelShader);
    }

    D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    result = m_pDevice->CreateInputLayout(inputDesc, 2, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout);

    if (pVSBlob) pVSBlob->Release();
    if (pPSBlob) pPSBlob->Release();

    return result;
}

HRESULT InitDirectX(HWND hWnd) {
    HRESULT result;
    IDXGIFactory* pFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(result)) return result;

    IDXGIAdapter* pSelectedAdapter = NULL;
    IDXGIAdapter* pAdapter = NULL;
    UINT adapterIdx = 0;
    while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter))) {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);
        if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) {
            pSelectedAdapter = pAdapter;
            break;
        }
        pAdapter->Release();
        adapterIdx++;
    }
    if (!pSelectedAdapter) return E_FAIL;

    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
        flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice, NULL, &m_pDeviceContext);
    pSelectedAdapter->Release();
    if (FAILED(result)) return result;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
    pFactory->Release();
    if (FAILED(result)) return result;

    result = CreateBackBuffer();
    if (FAILED(result)) return result;

    return InitScene();
}

void Render() {
    if (!m_pDeviceContext || !m_pBackBufferRTV) return;

    static const FLOAT BackColor[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
    //static const FLOAT BackColor[4] = { 0.25f, 0.25f, 1.0f };

    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);
    m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, nullptr);

    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)m_width, (FLOAT)m_height, 0.0f, 1.0f };
    m_pDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect = { 0, 0, m_width, m_height };
    m_pDeviceContext->RSSetScissorRects(1, &rect);

    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    m_pDeviceContext->DrawIndexed(3, 0, 0);

    m_pSwapChain->Present(0, 0);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (m_pSwapChain && m_pDevice) {
            if (m_pBackBufferRTV) m_pBackBufferRTV->Release();
            m_width = LOWORD(lParam);
            m_height = HIWORD(lParam);
            m_pSwapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
            CreateBackBuffer();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"DX11WindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    RECT rc = { 0, 0, m_width, m_height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, L"3.Triangle", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance, NULL);

    if (hWnd == NULL) return 0;
    ShowWindow(hWnd, nCmdShow);

    if (FAILED(InitDirectX(hWnd))) return -1;

    MSG msg = { };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Render();
        }
    }

    if (m_pVertexBuffer) m_pVertexBuffer->Release();
    if (m_pIndexBuffer) m_pIndexBuffer->Release();
    if (m_pInputLayout) m_pInputLayout->Release();
    if (m_pVertexShader) m_pVertexShader->Release();
    if (m_pPixelShader) m_pPixelShader->Release();
    if (m_pBackBufferRTV) m_pBackBufferRTV->Release();
    
    if (m_pSwapChain) m_pSwapChain->Release();
        if (m_pDeviceContext) {
        m_pDeviceContext->ClearState(); 
        m_pDeviceContext->Release();
    }
    if (m_pDevice) m_pDevice->Release();
}

