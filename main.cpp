#include <Wincodec.h>             // we use WIC for saving images
#include <d3d9.h>                 // DirectX 9 header
#pragma comment(lib, "d3d9.lib")  // link to DirectX 9 library

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define RELEASE(__p) {if(__p!=nullptr){__p->Release();__p=nullptr;}}

HRESULT SavePixelsToFile32bppPBGRA(UINT width, UINT height, UINT stride, LPBYTE pixels, LPWSTR filePath, const GUID& format)
{
    if (!filePath || !pixels)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    IWICImagingFactory* factory = nullptr;
    IWICBitmapEncoder* encoder = nullptr;
    IWICBitmapFrameEncode* frame = nullptr;
    IWICStream* stream = nullptr;
    GUID pf = GUID_WICPixelFormat32bppPBGRA;
    BOOL coInit = CoInitialize(nullptr);

    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    factory->CreateStream(&stream);
    stream->InitializeFromFilename(filePath, GENERIC_WRITE);
    factory->CreateEncoder(format, nullptr, &encoder);
    encoder->Initialize(stream, WICBitmapEncoderNoCache);
    encoder->CreateNewFrame(&frame, nullptr); // we don't use options here
    frame->Initialize(nullptr); // we dont' use any options here
    frame->SetSize(width, height);
    frame->SetPixelFormat(&pf);
    frame->WritePixels(height, stride, stride * height, pixels);
    frame->Commit();
    encoder->Commit();

    cleanup:
    RELEASE(stream);
    RELEASE(frame);
    RELEASE(encoder);
    RELEASE(factory);
    if (coInit) CoUninitialize();
    return hr;
}
HRESULT Direct3D9TakeScreenshots(UINT adapter, UINT count)
{
    HRESULT hr = S_OK;
    IDirect3D9* d3d = nullptr;
    IDirect3DDevice9* device = nullptr;
    IDirect3DSurface9* surface = nullptr;
    D3DPRESENT_PARAMETERS parameters = { 0 };
    D3DDISPLAYMODE mode;
    D3DLOCKED_RECT rc;
    UINT pitch;
    SYSTEMTIME st;
    LPBYTE* shots = nullptr;

    // init D3D and get screen size
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    d3d->GetAdapterDisplayMode(adapter, &mode);

    parameters.Windowed = TRUE;
    parameters.BackBufferCount = 1;
    parameters.BackBufferHeight = mode.Height;
    parameters.BackBufferWidth = mode.Width;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow = NULL;

    // create device & capture surface
    d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device);
    device->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, nullptr);

    // compute the required buffer size
    surface->LockRect(&rc, NULL, 0);
    pitch = rc.Pitch;
    surface->UnlockRect();

    // allocate screenshots buffers
    shots = new LPBYTE[count];
    for (UINT i = 0; i < count; i++)
    {
        shots[i] = new BYTE[pitch * mode.Height];
    }

    GetSystemTime(&st); // measure the time we spend doing <count> captures
   
    for (UINT i = 0; i < count; i++)
    {
        // get the data
        device->GetFrontBufferData(0, surface);

        // copy it into our buffers
        surface->LockRect(&rc, NULL, 0);
        CopyMemory(shots[i], rc.pBits, rc.Pitch * mode.Height);
        surface->UnlockRect();
    }
    GetSystemTime(&st);
    

    // save all screenshots
    for (UINT i = 0; i < count; i++)
    {
        WCHAR file[100];
        wsprintf(file, L"cap%i.png", i);
        SavePixelsToFile32bppPBGRA(mode.Width, mode.Height, pitch, shots[i], file, GUID_ContainerFormatPng);
    }

cleanup:
    if (shots != nullptr)
    {
        for (UINT i = 0; i < count; i++)
        {
            delete shots[i];
        }
        delete[] shots;
    }
    RELEASE(surface);
    RELEASE(device);
    RELEASE(d3d);
    return hr;
}
int main()
{
    HRESULT hr = Direct3D9TakeScreenshots(D3DADAPTER_DEFAULT, 10);
    return 0;
}