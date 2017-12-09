#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM
#include <stdio.h>
#include <math.h>

#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

// for compiling shaders
// could pre-compile shaders to avoid the need for this?
#include <D3Dcompiler.h>
// #pragma comment(lib, "D3dcompiler_47.lib") // load a dll instead



// hoops to avoid installing the sdk
// basically to get D3DCompile

// typedef struct {
//     LPCSTR Name;
//     LPCSTR Definition;
// } D3D_SHADER_MACRO;

DEFINE_GUID(IID_ID3D10Blob, 0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

typedef struct ID3D10BlobVtbl {
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )
            (ID3D10Blob * This, REFIID riid, void **ppvObject);

    ULONG   ( STDMETHODCALLTYPE *AddRef )(ID3D10Blob * This);
    ULONG   ( STDMETHODCALLTYPE *Release )(ID3D10Blob * This);
    LPVOID  ( STDMETHODCALLTYPE *GetBufferPointer )(ID3D10Blob * This);
    SIZE_T  ( STDMETHODCALLTYPE *GetBufferSize )(ID3D10Blob * This);

    END_INTERFACE
} ID3D10BlobVtbl;

#define ID3D10Blob_QueryInterface(This,riid,ppvObject) \
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define ID3D10Blob_AddRef(This) \
    ( (This)->lpVtbl -> AddRef(This) )

#define ID3D10Blob_Release(This) \
    ( (This)->lpVtbl -> Release(This) )

#define ID3D10Blob_GetBufferPointer(This) \
    ( (This)->lpVtbl -> GetBufferPointer(This) )

#define ID3D10Blob_GetBufferSize(This) \
    ( (This)->lpVtbl -> GetBufferSize(This) )

// interface ID3D10Blob
// {
//     CONST_VTBL struct ID3D10BlobVtbl *lpVtbl;
// };

typedef ID3D10Blob ID3DBlob;

typedef HRESULT (WINAPI *COMPILE_FUNC)
    (LPCVOID                         pSrcData,
     SIZE_T                          SrcDataSize,
     LPCSTR                          pFileName,
     CONST D3D_SHADER_MACRO*         pDefines,
     void* /*ID3DInclude* */         pInclude,
     LPCSTR                          pEntrypoint,
     LPCSTR                          pTarget,
     UINT                            Flags1,
     UINT                            Flags2,
     ID3DBlob**                      ppCode,
     ID3DBlob**                      ppErrorMsgs);

COMPILE_FUNC myD3DCompile;


float GetWallClockSeconds()
{
    LARGE_INTEGER counter;  QueryPerformanceCounter(&counter);
    LARGE_INTEGER freq;     QueryPerformanceFrequency(&freq);
    return (float)counter.QuadPart / (float)freq.QuadPart;
}


#define MULTILINE_STRING(...) #__VA_ARGS__


char *vertex_shader = MULTILINE_STRING
(
    struct VIN
    {
        float4 Position   : POSITION;
        float2 Texture    : TEXCOORD0;
    };
    struct VOUT
    {
        float4 Position   : POSITION;
        float2 Texture    : TEXCOORD0;
    };
    VOUT vs_main(in VIN In)
    {
        VOUT Out;
        Out.Position = In.Position;
        Out.Texture  = In.Texture;
        return Out;
    }
);

char *pixel_shader = MULTILINE_STRING
(
    struct PIN
    {
        float4 Position   : POSITION;
        float2 Texture    : TEXCOORD0;
    };
    struct POUT
    {
        float4 Color   : COLOR0;
    };
    sampler2D Tex0;
    POUT ps_main(in PIN In)
    {
        POUT Out;
        Out.Color = tex2D(Tex0, In.Texture);
        // Out.Color *= float4(0.9f, 0.8f, 0.4, 1);
        // Out.Color = float4(1, 0, 0, 1);
        return Out;
    }
);


bool load_directx()
{
    HINSTANCE dll = LoadLibrary("D3dcompiler_47.dll");
    if (!dll) { MessageBox(0, "Unable to load dll", 0, 0); return false; }

    myD3DCompile = (COMPILE_FUNC)GetProcAddress(dll, "D3DCompile");

    return true;
}


IDirect3D9* context;
IDirect3DDevice9* device;

bool init_directx(HWND win, int w, int h)
{
    context = Direct3DCreate9(D3D_SDK_VERSION);
    if (!context) { MessageBox(0, "Error creating direct3D context", 0, 0); return false; }

    D3DPRESENT_PARAMETERS params = {0};
    params.BackBufferWidth = w;
    params.BackBufferWidth = h;
    params.BackBufferFormat = D3DFMT_A8R8G8B8;
    params.BackBufferCount = 1;
    params.MultiSampleType = D3DMULTISAMPLE_NONE;
    params.MultiSampleQuality = 0;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = win;
    params.Windowed = true;
    params.EnableAutoDepthStencil = true;
    params.AutoDepthStencilFormat = D3DFMT_D16;
    params.Flags = 0;
    params.FullScreen_RefreshRateInHz = 0; // must be 0 for windowed
    params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    context->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        win,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &params,
        &device);

    return true;
}

IDirect3DVertexShader9 *vs;
IDirect3DPixelShader9 *ps;

void compile_shaders()
{
    ID3DBlob *code;
    ID3DBlob *errors;

    myD3DCompile(vertex_shader, strlen(vertex_shader), 0, 0, 0, "vs_main", "vs_1_1", 0, 0, &code, &errors);
    if (errors) {
        OutputDebugString("\nvs errors\n");
        OutputDebugString((char*)errors->GetBufferPointer());
        OutputDebugString("\n");
    }
    device->CreateVertexShader((DWORD*)code->GetBufferPointer(), &vs);

    myD3DCompile(pixel_shader, strlen(pixel_shader), 0, 0, 0, "ps_main", "ps_2_0", 0, 0, &code, &errors);
    if (errors) {
        OutputDebugString("\nps errors\n");
        OutputDebugString((char*)errors->GetBufferPointer());
        OutputDebugString("\n");
    }
    device->CreatePixelShader((DWORD*)code->GetBufferPointer(), &ps);
}


void render_pattern_to_texture(IDirect3DTexture9 *t, float dt)
{
    static int running_t = 0; running_t += dt;
    D3DLOCKED_RECT rect;
    HRESULT res = t->LockRect(0, &rect, 0, 0);
    if (res != D3D_OK) MessageBox(0,"error LockRect", 0, 0);
    for (int x = 0; x < 400; x++)
    {
        for (int y = 0; y < 400; y++)
        {
            byte *b = (byte*)rect.pBits + ((400*y)+x)*4 + 0;
            byte *g = (byte*)rect.pBits + ((400*y)+x)*4 + 1;
            byte *r = (byte*)rect.pBits + ((400*y)+x)*4 + 2;
            byte *a = (byte*)rect.pBits + ((400*y)+x)*4 + 3;

            *a = 255;
            *r = y*(255.0/400.0);
            *g = ((-cos(running_t*2*3.141592 / 3000) + 1) / 2) * 255.0;
            *b = x*(255.0/400.0);
        }
    }
    t->UnlockRect(0);
}

IDirect3DVertexBuffer9 *vb;
IDirect3DTexture9 *tex;
IDirect3DVertexDeclaration9 *vertexDecl;

void create_quad()
{
    float verts[] = {
    //   x   y  z   u  v
        -1, -1, 0,  0, 0,
        -1,  1, 0,  1, 0,
         1, -1, 0,  0, 1,
         1,  1, 0,  1, 1
    };
    HRESULT res = device->CreateVertexBuffer(5 * 4 * sizeof(float), 0, 0, D3DPOOL_DEFAULT, &vb, 0);
    if (res != D3D_OK) MessageBox(0,"error creating vb", 0, 0);

    void *where_to_copy_to;
    vb->Lock(0, 0, &where_to_copy_to, 0);
    memcpy(where_to_copy_to, verts, 5 * 4 * sizeof(float));
    vb->Unlock();


    D3DVERTEXELEMENT9 decl[] =
    {
        {
            0, 0,
            D3DDECLTYPE_FLOAT3,
            D3DDECLMETHOD_DEFAULT,
            D3DDECLUSAGE_POSITION, 0
        },
        {
            0, 0,
            D3DDECLTYPE_FLOAT2,
            D3DDECLMETHOD_DEFAULT,
            D3DDECLUSAGE_TEXCOORD, 0
        },
        D3DDECL_END()
    };
    res = device->CreateVertexDeclaration(decl, &vertexDecl);
    if (res != D3D_OK) OutputDebugString("CreateVertexDeclaration error!\n");


    res = device->CreateTexture(400, 400, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, 0);
    if (res != D3D_OK) MessageBox(0,"error CreateTexture", 0, 0);
    render_pattern_to_texture(tex, 0);
}

void render()
{
    device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,150,200), 1.0f, 0);
    device->BeginScene();

    device->SetVertexDeclaration(vertexDecl);
    device->SetVertexShader(vs);
    device->SetPixelShader(ps);
    device->SetStreamSource(0, vb, 0, 5*sizeof(float));
    device->SetTexture(0, tex);
    device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    device->EndScene();
    device->Present(0, 0, 0, 0);
}

bool running = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CLOSE) running = false;
    if (uMsg ==  WM_NCHITTEST) {
        RECT win; if (!GetWindowRect(hwnd, &win)) return HTNOWHERE;
        POINT pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        POINT pad = { GetSystemMetrics(SM_CXFRAME), GetSystemMetrics(SM_CYFRAME) };
        bool left   = pos.x < win.left   + pad.x;
        bool right  = pos.x > win.right  - pad.x -1;  // win.right 1 pixel beyond window, right?
        bool top    = pos.y < win.top    + pad.y;
        bool bottom = pos.y > win.bottom - pad.y -1;
        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;
        return HTCAPTION;
    }
    if (uMsg ==  WM_SIZE) {
        if (device) device->Present(0, 0, 0, 0);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "app";
    if (!RegisterClass(&wc)) { MessageBox(0, "RegisterClass failed", 0, 0); return 1; }

    HWND hwnd = CreateWindowEx(
        0, "app", "title",
        WS_POPUP | WS_VISIBLE,
        // WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400, 0, 0, hInstance, 0);
    if (!hwnd) { MessageBox(0, "CreateWindowEx failed", 0, 0); return 1; }


    // HDC hdc = GetDC(hwnd);

    load_directx();

    init_directx(hwnd, 400, 400);

    compile_shaders();

    create_quad();


    while(running)
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        render_pattern_to_texture(tex, 16);
        render();

        Sleep(16);
    }

    // not sure if these need to be released or not
    if (device) device->Release();
    if (context) context->Release();
    if (vs) vs->Release();
    if (ps) ps->Release();
    if (tex) tex->Release();
    if (vertexDecl) vertexDecl->Release();
    if (vb) vb->Release();

    return 0;
}
