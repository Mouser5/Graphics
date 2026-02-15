#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <assert.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ID3D11Device* m_pDevice = nullptr;
ID3D11DeviceContext* m_pDeviceContext = nullptr;
IDXGISwapChain* m_pSwapChain = nullptr;
ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
int m_width = 1280;
int m_height = 720;

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

    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
        flags, levels, 1, D3D11_SDK_VERSION, &m_pDevice, &level, &m_pDeviceContext);

    pSelectedAdapter->Release();
    if (FAILED(result)) return result;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    result = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
    pFactory->Release();
    if (FAILED(result)) return result;

    return CreateBackBuffer();
}

void Render() {
    if (!m_pDeviceContext || !m_pBackBufferRTV) return;
    // В презентации  static const FLOAT BackColor[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
    static const FLOAT BackColor[4] = { 0.25f, 0.25f, 1.0f };
    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, BackColor);

    m_pDeviceContext->OMSetRenderTargets(1, &m_pBackBufferRTV, nullptr);

    m_pSwapChain->Present(0, 0);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (m_pSwapChain && m_pDevice) {
            if (m_pBackBufferRTV) {
                m_pBackBufferRTV->Release();
                m_pBackBufferRTV = nullptr;
            }

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

    HWND hWnd = CreateWindowEx(
        0, CLASS_NAME, L"DirectX 11 Initialization Task",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) return 0;

    ShowWindow(hWnd, nCmdShow);

    if (FAILED(InitDirectX(hWnd))) {
        return -1;
    }

    MSG msg = { };
    bool exit = false;
    while (!exit) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                exit = true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Render();
        }
    }

    if (m_pBackBufferRTV) m_pBackBufferRTV->Release();
    if (m_pSwapChain) m_pSwapChain->Release();
    if (m_pDeviceContext) m_pDeviceContext->Release();
    if (m_pDevice) {
        ID3D11Debug* d3dDebug = nullptr;
        m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
        m_pDevice->Release();
        if (d3dDebug) {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
            d3dDebug->Release();
        }
    }

    return 0;
}