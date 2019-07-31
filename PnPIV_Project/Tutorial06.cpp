
#include <windows.h>
#include <windowsx.h>
#include <d3d11_1.h>

#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include <fbxsdk.h>
#include <iostream>
#include <vector>
#include <codecvt>
#include <locale>
#include <string>
#include "DDSTextureLoader.h"

#include <dinput.h>
#include "dirent.h"
using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
	XMFLOAT2 Tex;
};
struct ConstantBuffer
{
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
};
struct ConstantBuffer2
{
	XMFLOAT4 vLightDir[3];
	XMFLOAT4 vLightColor[3];
	XMFLOAT4 vLightPos[3];
	XMFLOAT4 vLightType[3];
	XMFLOAT4 vLightRange[3];
	XMFLOAT4 vInnerAngle[3];
	XMFLOAT4 vOuterAngle[3];
	XMFLOAT4 vOutputColor;
};
struct Part
{
	const char * name;
	const char * meshName;
	std::string textureName;
	const char * materialName;

	UINT VertCount = 0;
	UINT IndiceCount = 0;

	std::vector<SimpleVertex> verts;
	std::vector<UINT> indices;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	ID3D11ShaderResourceView * texture = nullptr;

	XMMATRIX Part_WorldMat = XMMatrixIdentity();
};
struct Model
{
	const char* ModelName;
	std::vector<Part> parts;
	XMMATRIX Model_WorldM = XMMatrixIdentity();
};
struct SkyBox
{
	UINT VertCount = 0;
	UINT IndiceCount = 0;

	std::vector<SimpleVertex> verts;
	std::vector<UINT> indices;

	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	ID3D11ShaderResourceView * texture = nullptr;

	XMMATRIX World_M = XMMatrixIdentity();
};
struct Light
{
	XMFLOAT4 LightDirection = { 0,0,0,0 };
	XMFLOAT4 LightColor = { 0,0,0,0 };
	XMFLOAT4 LightPos = { 0,0,0,0 };
	int LightType = -1;
	float LightRange = 0.0f;
	float InnerAngle = 0.0f;
	float OuterAngle = 0.0f;
};
//Lights
//-1 default
// 0 directional
// 1 point
// 2 Spot


#define RAND_NORMAL XMFLOAT3(rand()/float(RAND_MAX),rand()/float(RAND_MAX),rand()/float(RAND_MAX))

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = nullptr;
HWND                    g_hWnd = nullptr;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11Device1*          g_pd3dDevice1 = nullptr;
ID3D11DeviceContext*    g_pImmediateContext = nullptr;
ID3D11DeviceContext1*   g_pImmediateContext1 = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
IDXGISwapChain1*        g_pSwapChain1 = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11Texture2D*        g_pDepthStencil = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
ID3D11VertexShader*     g_pVertexShader = nullptr;
ID3D11PixelShader*      g_pPixelShader = nullptr;

ID3D11VertexShader*     g_pCubeMapVS = nullptr;
ID3D11PixelShader*      g_pCubeMapPS = nullptr;
ID3D11InputLayout*      g_pCubeMapVertexLayout = nullptr;

ID3D11PixelShader*      g_pPixelShaderSolid = nullptr;
ID3D11InputLayout*      g_pVertexLayout = nullptr;


ID3D11Buffer*           g_pVertexBuffer = nullptr;
ID3D11Buffer*           g_pIndexBuffer = nullptr;
ID3D11Buffer*           g_pConstantBuffer = nullptr;
ID3D11Buffer*           g_pConstantBuffer2 = nullptr;
XMMATRIX                g_World;
XMMATRIX                g_View;
XMMATRIX                g_Projection;

ID3D11ShaderResourceView*           g_pTextureRV = nullptr;
ID3D11SamplerState*                 g_pSamplerLinear = nullptr;
ID3D11SamplerState*					g_pSamplerTrilinear = nullptr;
ID3D11Buffer*                       g_pCBNeverChanges = nullptr;
ID3D11Buffer*                       g_pCBChangeOnResize = nullptr;
ID3D11Buffer*                       g_pCBChangesEveryFrame = nullptr;

XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);
XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

UINT width;
UINT height;
LPNCCALCSIZE_PARAMS pncc;

XMVECTOR DefaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR DefaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR DefaultUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
XMVECTOR camForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR camRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

XMVECTOR camUp = Up;
XMVECTOR camPosition = Eye;
XMVECTOR camTarget = At;
XMMATRIX camRotationMatrix;
XMMATRIX groundWorld;

SimpleVertex *vertices;
int numVertices = 0;
int *indices;
int numIndices = 0;
int numUVs = 0;
float scale = 2.0f;
float pitch = 0.0f;
float yaw = 0.0f;
float moveLeftRight = 0.0f;
float moveBackForward = 0.0f;
float moveUpDown = 0.0f;
bool CursorLock = true;
float DefaultCursorPosX = 960;
float DefaultCursorPosY = 540;
int SwitchLight = 1;
DIMOUSESTATE mouseCurrState;
DIMOUSESTATE mouseLastState;
XMVECTOR Move = { 0.0f, 0.0f, 0.0f };
XMMATRIX Translation = XMMatrixIdentity();
RECT rc;

XMMATRIX NinetyDegXRot = XMMatrixIdentity() * XMMatrixRotationX(1.273);;
XMMATRIX TimeYRotMed;
XMMATRIX TimeYRotSlow;
XMMATRIX tempEarth;
XMMATRIX tempMars;
XMMATRIX tempMercury;
XMMATRIX tempVenus;
XMMATRIX MoonTrans = XMMatrixIdentity() * XMMatrixTranslation(10.0f, 0.0f, 0.0f);
XMMATRIX ShipTrans = XMMatrixIdentity() * XMMatrixTranslation(5.0f, 0.0f, 0.0f);


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );
void Render();
void ProcessFbxMesh(FbxNode * Node);
void list_dir(const char *path);
XMMATRIX UpdateViewMatrix();
Part CreateSphere(int LatLines, int LongLines, float Scale);
Part CreateGrid(float width, float depth, UINT n, UINT m, float angleX, float angleY, float angleZ);
//-------------------------------------------------------------------------------------------------------------//

//-------------------------------------------------------------------------------------------------------------//
//Vectors Parts, Models, and Lights
std::vector<Part> ModelParts;
std::vector<Model> Models;
std::vector<Light> Lights;
std::vector<std::string> objects;
//-------------------------------------------------------------------------------------------------------------//

//-------------------------------------------------------------------------------------------------------------//
//Foward Declarations of Render Functions
void RenderModel(Model mod);
void RenderSkyBox(Model SkyBox);
//-------------------------------------------------------------------------------------------------------------//

//-------------------------------------------------------------------------------------------------------------//
//Global Variables
Part Skybox;
Model Sky;
XMMATRIX temp1;

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

    if( FAILED( InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    if( FAILED( InitDevice() ) )
    {
        CleanupDevice();
        return 0;
    }

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
			
        }
        else
        {
            Render();
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;
    // Create window
    g_hInst = hInstance;
    rc = { 0, 0, 1920, 1080 };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"Direct3D 11 Engine",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX | WM_SIZE, 
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                           nullptr );
    if( !g_hWnd )
        return E_FAIL;
    ShowWindow( g_hWnd, nCmdShow );
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile( const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut )
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile( szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob );
    if( FAILED(hr) )
    {
        if( pErrorBlob )
        {
            OutputDebugStringA( reinterpret_cast<const char*>( pErrorBlob->GetBufferPointer() ) );
            pErrorBlob->Release();
        }
        return hr;
    }
    if( pErrorBlob ) pErrorBlob->Release();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	//enable hr flag for debugger to work
	//in the directx3d executable list add in the root folder for project and leave application controlled box checked
	//after that go into break setting tab and check corruption and errors and the enable break functionality

    HRESULT hr = S_OK;

    
    GetClientRect( g_hWnd, &rc );
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );

        if ( hr == E_INVALIDARG )
        {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice( nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                                    D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        }

        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface( __uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice) );
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent( __uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory) );
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd = {};
		//why is this set to 1?
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = g_hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( g_pd3dDevice, &sd, &g_pSwapChain );
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation( g_hWnd, DXGI_MWA_NO_ALT_ENTER );
    dxgiFactory->Release();
    if (FAILED(hr))
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
    // Create a render target view

	ID3D11Texture2D* pBackBuffer = nullptr;

    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;
    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	// Compile the regular vertex shader
	ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial06.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }
	// Create the regular vertex shader
	hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader );
	if( FAILED( hr ) )
	{	
		pVSBlob->Release();
        return hr;
	}
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Complie the CubeMap Vertex Shader
	ID3DBlob* cm_VSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial06.fx", "VSCubeMap", "vs_4_0", &cm_VSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	// Create the CubeMap Vertex Shader
	hr = g_pd3dDevice->CreateVertexShader(cm_VSBlob->GetBufferPointer(), cm_VSBlob->GetBufferSize(), nullptr, &g_pCubeMapVS);
	if (FAILED(hr))
	{
		cm_VSBlob->Release();
		return hr;
	}
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE( layout );
    // Create the input layout
	hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
	pVSBlob->Release();
	if( FAILED( hr ) )
        return hr;
    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	// Compile the regular pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial06.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }
	// Create the regular pixel shader
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Compile CubeMap Pixel Shader
	pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"Tutorial06.fx", "PSCubeMap", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	// Create the CubeMap Pixel Shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pCubeMapPS);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Compile the PSSolid Pixel shader
	pPSBlob = nullptr;
	hr = CompileShaderFromFile( L"Tutorial06.fx", "PSSolid", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }
	//Create the PSSolid Pixel shader
	hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShaderSolid );
	pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Read In and Process Objects From File
	const char* path = "C:/Users/nate2/OneDrive/Desktop/P&PIV Repo/PnPIV_Project/Objects/";
	list_dir(path);
	FbxScene* lScene;
	Model mod;
	const char* lFilename;
	const char* passedValue;
	std::string Front = "Objects/";
	for (size_t i = 0; i < objects.size(); i++)
	{
		passedValue = "";
		lFilename = "";
		lFilename = objects[i].data();
		if (i == 1 || i == 2)
		{
			scale = 10.0f;
		}
		else
		{
			scale = 1.0f;
		}
		Front = "Objects/";
		Front += lFilename;
		passedValue = Front.c_str();
		//const char* lFilename = "GunFBX.fbx";
		// Initialize the SDK manager. This object handles all our memory management.
		FbxManager* lSdkManager = FbxManager::Create();
		// Create the IO settings object.
		FbxIOSettings *ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
		lSdkManager->SetIOSettings(ios);
		// Create an importer using the SDK manager.
		FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
		// Use the first argument as the filename for the importer.
		if (!lImporter->Initialize(passedValue, -1, lSdkManager->GetIOSettings())) {
			printf("Call to FbxImporter::Initialize() failed.\n");
			printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
			exit(-1);
		}
		// Create a new scene so that it can be populated by the imported file.
		lScene = FbxScene::Create(lSdkManager, "myScene");
		// Import the contents of the file into the scene.
		lImporter->Import(lScene);
		// The file is imported; so get rid of the importer.
		lImporter->Destroy();
		ProcessFbxMesh(lScene->GetRootNode());
		mod.parts = ModelParts;
		mod.ModelName = ModelParts[0].meshName;
		if (i != objects.size() - 1)
		{
			Models.push_back(mod);
		}
		else
		{
			Skybox = mod.parts[0];
		}
		ModelParts.clear();
	}

	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Buffer Creation for all File Read Objects
	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;
	for (size_t i = 0; i < Models.size(); i++)
	{
		for (size_t j = 0; j < Models[i].parts.size(); j++)
		{
			//VertexBuffers
			bd = {};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(SimpleVertex) * Models[i].parts[j].VertCount;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			InitData = {};
			InitData.pSysMem = Models[i].parts[j].verts.data();
			hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Models[i].parts[j].VertexBuffer);
			if (FAILED(hr))
				return hr;
			//Set Index Buffers
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(int) * Models[i].parts[j].IndiceCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;
			InitData.pSysMem = Models[i].parts[j].indices.data();
			hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Models[i].parts[j].IndexBuffer);
			if (FAILED(hr))
				return hr;
			//Read Texture From Object
			std::string Folder = "Textures/";
			Folder += Models[i].parts[j].textureName;
			std::wstring widestr = std::wstring(Folder.begin(), Folder.end());
			const wchar_t* widecstr = widestr.c_str();
			hr = CreateDDSTextureFromFile(g_pd3dDevice, widecstr, nullptr, &Models[i].parts[j].texture);
			if (FAILED(hr))
				return hr;
			Folder = "";
		}
	}
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Create Grid
	Part temp = CreateGrid(200, 200, 10, 10, 0, 0, 0);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * temp.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = temp.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &temp.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * temp.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = temp.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &temp.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/MetalFloor.dds", nullptr, &temp.texture);
	if (FAILED(hr))
		return hr;
	Model Hold;
	Hold.parts.push_back(temp);
	Hold.ModelName = "Grid";
	Models.push_back(Hold);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of Earth
	Part Sphere = CreateSphere(200, 200, 4.0f);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Sphere.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Sphere.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Sphere.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Sphere.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Sphere.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Sphere.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/newEarthtex.dds", nullptr, &Sphere.texture);
	if (FAILED(hr))
		return hr;
	Model Sph;
	Sph.parts.push_back(Sphere);
	Sph.ModelName = "Earth";
	Models.push_back(Sph);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of the Moon
	Part Moon = CreateSphere(100, 100, 1.0f);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Moon.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Moon.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Moon.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Moon.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Moon.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Moon.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/Moon.dds", nullptr, &Moon.texture);
	if (FAILED(hr))
		return hr;
	Model MoonM;
	MoonM.parts.push_back(Moon);
	MoonM.ModelName = "Moon";
	Models.push_back(MoonM);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of the Sun
	Part Sun = CreateSphere(200, 200, 10.0f);
	for (size_t i = 0; i < Sun.verts.size(); i++)
	{
		Sun.verts[i].Normal.x = Sun.verts[i].Pos.x * -1.0f;
		Sun.verts[i].Normal.y = Sun.verts[i].Pos.y * -1.0f;
		Sun.verts[i].Normal.z = Sun.verts[i].Pos.z * -1.0f;
	}
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Sun.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Sun.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Sun.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Sun.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Sun.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Sun.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/SunTex.dds", nullptr, &Sun.texture);
	if (FAILED(hr))
		return hr;
	Model SunM;
	SunM.parts.push_back(Sun);
	SunM.ModelName = "Sun";
	Models.push_back(SunM);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of Mars
	Part Mars = CreateSphere(100, 100, 2.0f);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Mars.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Mars.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Mars.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Mars.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Mars.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Mars.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/MarsTex.dds", nullptr, &Mars.texture);
	if (FAILED(hr))
		return hr;
	Model MarsM;
	MarsM.parts.push_back(Mars);
	MarsM.ModelName = "Mars";
	Models.push_back(MarsM);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of Mercury
	Part Mercury = CreateSphere(100, 100, 0.75f);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Mercury.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Mercury.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Mercury.VertexBuffer);
	if (FAILED(hr))
		return hr;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Mercury.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Mercury.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Mercury.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/Mercury.dds", nullptr, &Mercury.texture);
	if (FAILED(hr))
		return hr;
	Model MercuryM;
	MercuryM.parts.push_back(Mercury);
	MercuryM.ModelName = "Mercury";
	Models.push_back(MercuryM);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Creation of Venus
	Part Venus = CreateSphere(100, 100, 1.4f);
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Venus.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Venus.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Venus.VertexBuffer);
	if (FAILED(hr))
		return hr;
	// Pack the indices of all the meshes into one index buffer.
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Venus.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Venus.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Venus.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/Venus.dds", nullptr, &Venus.texture);
	if (FAILED(hr))
		return hr;
	Model VenusM;
	VenusM.parts.push_back(Venus);
	VenusM.ModelName = "Venus";
	Models.push_back(VenusM);
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Create and Invert Skybox from Rendering
	SimpleVertex trade;
	for (size_t i = 0; i < Skybox.verts.size(); i++)
	{
		trade = Skybox.verts[i];
		Skybox.verts[i] = Skybox.verts[i + 1];
		Skybox.verts[i + 1] = trade;
		i += 2;
	}
	for (size_t i = 0; i < Skybox.verts.size(); i++)
	{
		Skybox.verts[i].Pos.x *= 990.0f;
		Skybox.verts[i].Pos.y *= 990.0f;
		Skybox.verts[i].Pos.z *= 990.0f;
	}
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * Skybox.VertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData = {};
	InitData.pSysMem = Skybox.verts.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Skybox.VertexBuffer);
	if (FAILED(hr))
		return hr;
	// Pack the indices of all the meshes into one index buffer.
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(int) * Skybox.IndiceCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = Skybox.indices.data();
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &Skybox.IndexBuffer);
	if (FAILED(hr))
		return hr;
	hr = CreateDDSTextureFromFile(g_pd3dDevice, L"Textures/FinalStar1024.dds", nullptr, &Skybox.texture);
	if (FAILED(hr))
		return hr;
	Sky.parts.push_back(Skybox);
	Sky.ModelName = "SkyBox";
	Sky.Model_WorldM = XMMatrixIdentity();
	//-------------------------------------------------------------------------------------------------------------//
	//-------------------------------------------------------------------------------------------------------------//
	//Create Lights and Add to Vector
	Light DirectionalLight;
	DirectionalLight.LightDirection = XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	DirectionalLight.LightColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	DirectionalLight.LightType = 0;
	Lights.push_back(DirectionalLight);

	Light PointLight;
	PointLight.LightDirection = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	PointLight.LightColor = XMFLOAT4(1.0f, 1.0f, 0.75f, 1.0f);
	PointLight.LightPos = XMFLOAT4(0.0f, 40.0f, 0.0f, 1.0f);
	PointLight.LightType = 1;
	PointLight.LightRange = 5000.0f;
	Lights.push_back(PointLight);

	Light SpotLight;
	SpotLight.LightDirection = XMFLOAT4(0.0f, -1.0f, 0.0f, 1.0f);
	SpotLight.LightColor = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	SpotLight.LightPos = XMFLOAT4(0.0f, 5.0f, 0.0f, 1.0f);
	SpotLight.LightType = 2;
	//SpotLight.LightRange = 305.5f;
	SpotLight.InnerAngle = 0.75f;
	SpotLight.OuterAngle = 0.5f;
	Lights.push_back(SpotLight);
	
	// Create the constant buffer
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer2);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer2);
	if (FAILED(hr))
		return hr;

	// Create the constant buffer
	bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;
	sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerTrilinear);
	if (FAILED(hr))
		return hr;
	

	////////////////////////////////////////////////////////////////////////////////////////

    // Initialize the world matrices
	g_World = XMMatrixIdentity();

    // Initialize the view matrix
	
	g_View = XMMatrixLookAtLH( Eye, At, Up );
	
	
    // Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );

    return S_OK;
}

//void ProcessFbxMesh(FbxMesh * mesh, std::vector<SimpleVertex>& V, std::vector<UINT>& I) 
//{
//	AllocConsole();
//	freopen("CONOUT$", "w", stdout);
//	freopen("CONOUT$", "w", stderr);
//
//	//FBX Mesh stuff
//	
//	std::cout << "\nName:" << mesh->GetName();
//	std::cout << "\nVertexCount: " << mesh->GetControlPointsCount();
//	std::cout << "\nIndiceCount: " << mesh->GetPolygonVertexCount();
//	numVertices += mesh->GetControlPointsCount();
//	numIndices += mesh->GetPolygonVertexCount();
//
//	FbxVector4* ControlPoints = mesh->GetControlPoints();
//	FbxArray<FbxVector4> normalsVec;
//	mesh->GetPolygonVertexNormals(normalsVec);
//	FbxGeometryElementNormal* Normals = mesh->GetElementNormal();
//
//	int meshelementcount = (mesh->GetControlPointsCount() + mesh->GetElementNormalCount() + mesh->GetElementUVCount());
//	for (size_t i = 0; i < mesh->GetControlPointsCount(); i++)
//	{
//		V[i].Pos.x = ControlPoints[i][0];
//		V[i].Pos.y = ControlPoints[i][1];
//		V[i].Pos.z = ControlPoints[i][2];
//		V[i].Normal.x = normalsVec.GetAt(i)[0];
//		V[i].Normal.y = normalsVec.GetAt(i)[1];
//		V[i].Normal.z = normalsVec.GetAt(i)[2];	
//	}
//	FbxStringList lUVSetNameList;
//	mesh->GetUVSetNames(lUVSetNameList);
//	const FbxGeometryElementUV* lUVElement;
//	for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
//	{
//		const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
//		lUVElement = mesh->GetElementUV(lUVSetName);
//		for (int j = 0; j < V.size(); j++)
//		{
//			const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
//			int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(j) : j;
//
//			V[j].Tex.x = lUVElement->GetDirectArray().GetAt(lUVIndex)[0];
//			V[j].Tex.y = lUVElement->GetDirectArray().GetAt(lUVIndex)[1];
//		}
//	}
//	//for (size_t i = 0; i < V.size(); i++)
//	//{
//	//	/*std::cout << "\nPolygon Index: " << i;
//	//	std::cout << "\nPos X: " << V[i].Pos.x;
//	//	std::cout << "\nPos Y: " << V[i].Pos.y;
//	//	std::cout << "\nPos Z: " << V[i].Pos.z;
//	//	std::cout << "\nNormal X: " << V[i].Normal.x;
//	//	std::cout << "\nNormal Y: " << V[i].Normal.y;
//	//	std::cout << "\nNormal Z: " << V[i].Normal.z;
//	//	std::cout << "\nTexture U: " << V[i].Tex.x;
//	//	std::cout << "\nTexture V: " << V[i].Tex.y;	
//	//	std::cout << "\n";*/	
//
//	//}
//	Model mod;
//	mod.verts = V;
//	mod.indices = I;
//	
//	models.push_back(mod);
//}

void ProcessFbxMesh(FbxNode * Node)
{
	//Console to Write to
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	//Amount of Children
	int childrenCount = Node->GetChildCount();
	//Model Object to be added after information is gathered
	Part mod;
	SimpleVertex *vertices2;
	std::string materialName;
	std::string propName;
	FbxProperty* prop;

	//Mesh of Model
	FbxMesh* mesh;
	for (int i = 0; i < childrenCount; i++)
	{	
		FbxNode *childNode = Node->GetChild(i);
		mesh = childNode->GetMesh();	
		if (mesh != NULL)
		{
			FbxAMatrix lTransform;
			//Mesh Name
			mod.meshName = childNode->GetName();
			//Mesh Indice Count
			numIndices = mesh->GetPolygonVertexCount();
			//-------------------------------------------------------------------------------------//
			//Process the texture
			int materialCount = childNode->GetMaterialCount();
			int materialCount2 = Node->GetMaterialCount();
			if (materialCount > 0)
			{
				for (size_t i = 0; i < materialCount; i++)
				{
					FbxSurfaceMaterial *lMaterial = childNode->GetMaterial(i);
					mod.textureName = lMaterial->GetName();
				}
			}
			//for(int index = 0; index < materialCount; index++)
			//{
			//	/*FbxSurfaceMaterial* material = (FbxSurfaceMaterial*)childNode->GetSrcObject<FbxSurfaceMaterial>(index);
			//	if (material != nullptr)
			//	{
			//		mod.materialName = material->GetName();
			//		FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);	
			//		int textureCount = prop.GetSrcObjectCount<FbxTexture>();		
			//		for (int j = 0; j < textureCount; j++)
			//		{
			//			FbxFileTexture* texture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>(j));
			//			
			//			std::string p = texture->GetName();
			//			mod.textureName = p;
			//		}		
			//	}*/
			//}
			//-------------------------------------------------------------------------------------//
			// No need to allocate int array, FBX does for us
			indices = mesh->GetPolygonVertices();
			
			// Get vertex count from mesh
			numVertices = numVertices + mesh->GetControlPointsCount();
			int meshverts = mesh->GetControlPointsCount();

			//-------------------------------------------------------------------------------------//
			//X,Y,Z Positions of Vertices	
			vertices = new SimpleVertex[numVertices];
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);				
				vertices[j].Pos.x = (float)vert.mData[0] / scale;
				vertices[j].Pos.y = (float)vert.mData[1] / scale;
				vertices[j].Pos.z = (float)vert.mData[2] / scale;				
			}
			//-------------------------------------------------------------------------------------//

			//-------------------------------------------------------------------------------------//
			//Getting Normals and UVs
			FbxArray<FbxVector2> UVsVec;
			vertices2 = new SimpleVertex[numIndices];
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			FbxStringList lUVSetNameList;
			mesh->GetUVSetNames(lUVSetNameList);
			const FbxGeometryElementUV* lUVElement;
			for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
			{
				const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
				lUVElement = mesh->GetElementUV(lUVSetName);
				for (int j = 0; j < numIndices; j++)
				{
					//Normals
					vertices2[j] = vertices[indices[j]];
					vertices2[j].Normal.x = normalsVec.GetAt(j)[0];
					vertices2[j].Normal.y = normalsVec.GetAt(j)[1];
					vertices2[j].Normal.z = normalsVec.GetAt(j)[2];
					//UVs
					const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
					int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(j) : j;
					vertices2[j].Tex.x = lUVElement->GetDirectArray().GetAt(lUVIndex)[0];
					vertices2[j].Tex.y = 1.0f - lUVElement->GetDirectArray().GetAt(lUVIndex)[1];
				}
			}
			//-------------------------------------------------------------------------------------//
			
			delete vertices;

			// make new indices to match the new vertex(2) array
			delete indices;
			indices = new int[numIndices];	
			for (int j = 0; j < numIndices; j++)
			{
				indices[j] = j;
				mod.indices.push_back(indices[j]);
			}
			numVertices = numIndices;	

			mod.name = Node->GetName();

			mod.VertCount = numVertices;

			mod.IndiceCount = numIndices;
			for (size_t i = 0; i < numVertices; i++)
			{
				mod.verts.push_back(vertices2[i]);
			}

			int vertsvectorcount = 0;
			vertsvectorcount = mod.verts.size();

			ModelParts.push_back(mod);
		}	
		//-------------------------------------------------------------------------------------//
		//Add Information into the model		
		ProcessFbxMesh(childNode);
	}
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
    if( g_pImmediateContext ) g_pImmediateContext->ClearState();

	//////////////////////////
	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pSamplerTrilinear) g_pSamplerTrilinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	/////////////////////////
	if (g_pConstantBuffer) g_pConstantBuffer->Release();
	if( g_pConstantBuffer2 ) g_pConstantBuffer2->Release();
	for (size_t i = 0; i < Models.size(); i++)
	{
		for (size_t j = 0; j < Models[i].parts.size(); j++)
		{
			if (Models[i].parts[j].VertexBuffer)
			{
				Models[i].parts[j].VertexBuffer->Release();
			}
			if (Models[i].parts[j].IndexBuffer)
			{
				Models[i].parts[j].IndexBuffer->Release();
			}
			if (Models[i].parts[j].texture)
			{
				Models[i].parts[j].texture->Release();
			}
		}
	}
	if (Sky.parts[0].VertexBuffer)
	{
		Sky.parts[0].VertexBuffer->Release();
	}
	if (Sky.parts[0].IndexBuffer)
	{
		Sky.parts[0].IndexBuffer->Release();
	}
	if (Sky.parts[0].texture)
	{
		Sky.parts[0].texture->Release();
	}
    if( g_pVertexBuffer ) g_pVertexBuffer->Release();
    if( g_pIndexBuffer ) g_pIndexBuffer->Release();
    if( g_pVertexLayout ) g_pVertexLayout->Release();
	if (g_pCubeMapVertexLayout) g_pCubeMapVertexLayout->Release();
    if( g_pVertexShader ) g_pVertexShader->Release();
	if (g_pCubeMapVS) g_pCubeMapVS->Release();

    if( g_pPixelShaderSolid ) g_pPixelShaderSolid->Release();
    if( g_pPixelShader ) g_pPixelShader->Release();
	if (g_pCubeMapPS) g_pCubeMapPS->Release();

    if( g_pDepthStencil ) g_pDepthStencil->Release();
    if( g_pDepthStencilView ) g_pDepthStencilView->Release();
    if( g_pRenderTargetView ) g_pRenderTargetView->Release();
    if( g_pSwapChain1 ) g_pSwapChain1->Release();
    if( g_pSwapChain ) g_pSwapChain->Release();
    if( g_pImmediateContext1 ) g_pImmediateContext1->Release();
    if( g_pImmediateContext ) g_pImmediateContext->Release();
    if( g_pd3dDevice1 ) g_pd3dDevice1->Release();
    if( g_pd3dDevice ) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;
	pncc = (LPNCCALCSIZE_PARAMS)lParam;
	static bool s_in_sizemove = false;
	static bool s_minimized = false;

	
    switch( message )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
		
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;
	/*case WM_NCCALCSIZE:
	{
		const int cxBorder = 2;
		const int cyBorder = 2;
		InflateRect((LPRECT)lParam, -cxBorder, -cyBorder);
	}*/

	/////////////////////////////////////
	case WM_SIZE:
		
		/*width = LOWORD(lParam);
		height = HIWORD(lParam);*/
		if (wParam == SIZE_MINIMIZED)
		{
			// The window was minimized (you should probably suspend the application)
			if (!s_minimized)
			{
				s_minimized = true;
			}
		}
		else if (s_minimized)
		{
			// The window was minimized and is now restored (resume from suspend)
			s_minimized = false;
		}
		else if (!s_in_sizemove)
		{
			rc.right = LOWORD(lParam);
			
			rc.bottom = HIWORD(lParam);
			
			// Here is where you handle the swapchain resize for maximize or unmaximize
		}
		
		break;

	case WM_ENTERSIZEMOVE:
		// We want to avoid trying to resizing the swapchain as the user does the 'rubber band' resize
		s_in_sizemove = true;
		break;

	case WM_EXITSIZEMOVE:
		s_in_sizemove = false;
		// Here is the other place where you handle the swapchain resize after the user stops using the 'rubber-band' 
		break;

	case WM_GETMINMAXINFO:
	{
		// We want to prevent the window from being set too tiny
		auto info = reinterpret_cast<MINMAXINFO*>(lParam);
		info->ptMinTrackSize.x = 320;
		info->ptMinTrackSize.y = 200;
		
	}
	break;
	//////////////////////////////////////////
        

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	width = rc.right - rc.left;
	height = rc.bottom - rc.top;
    // Update our time
    static float t = 0.0f;
    if( g_driverType == D3D_DRIVER_TYPE_REFERENCE )
    {
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) * 0.001f;
	}

	//-------------------------------------------------------------------------------------------------------------//
	// Update Rotation and Positions of Objects
	TimeYRotMed  = NinetyDegXRot * XMMatrixRotationY(t * 0.5f);
	TimeYRotSlow = NinetyDegXRot * XMMatrixRotationY(t * 0.25f);
	
	//Planet Rotation and Translation
	tempEarth    = TimeYRotSlow * XMMatrixTranslation(100.0f, 40.0f, 0.0f);
	tempMars     = TimeYRotMed * XMMatrixTranslation(200.0f, 40.0f, 0.0f);
	tempMercury  = TimeYRotMed * XMMatrixTranslation(35.0f, 40.0f, 0.0f);
	tempVenus    = TimeYRotMed * XMMatrixTranslation(65.0f, 40.0f, 0.0f);

	//Mars
	g_World = XMMatrixRotationY(t * 0.025f);
	Models[9].Model_WorldM = tempMars * g_World;
	//Alien Ship
	g_World = XMMatrixRotationY(t * -0.5f);
	Models[0].Model_WorldM = ShipTrans * g_World * Models[9].Model_WorldM;
	//Mercury
	g_World = XMMatrixRotationY(t * 0.025f);
	Models[10].Model_WorldM = tempMercury * g_World;
	//Venus
	g_World = XMMatrixRotationY(t * 0.075f);
	Models[11].Model_WorldM = tempVenus * g_World;
	//Earth
	g_World = XMMatrixRotationY(t * 0.05f);
	Models[6].Model_WorldM = tempEarth * g_World;
	//Moon
	g_World = XMMatrixRotationY(t * 0.025f);
	Models[7].Model_WorldM = MoonTrans * g_World * (XMMatrixRotationX(-1.273) * Models[6].Model_WorldM);
	g_World = XMMatrixRotationY(t * 0.05f);
	//Sun
	Models[8].Model_WorldM = XMMatrixIdentity() * XMMatrixRotationX(1.273) * XMMatrixTranslation(0.0f, 40.0f, 0.0f) * g_World;

	int x = -4;
	g_World = XMMatrixRotationY(t);
	for (size_t i = 0; i < 5; i++)
	{
		Models[i].Model_WorldM = XMMatrixIdentity() * g_World * XMMatrixTranslation(x, 0.0f, 0.0f);
		x += 2;
	}
	
	//-------------------------------------------------------------------------------------------------------------//
	//Controls for Moving Lights and Light Selection
	//-------------------------------------------------------------------------------------------------------------//
	if (GetAsyncKeyState('1'))
	{
		SwitchLight++;
		if (SwitchLight > 2)
		{
			SwitchLight = 2;
		}
	}
	if (GetAsyncKeyState('2'))
	{
		SwitchLight--;
		if (SwitchLight < 1)
		{
			SwitchLight = 1;
		}
	}
	if (GetAsyncKeyState(VK_LEFT))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.x -= 0.025f;
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.x -= 0.025f;
		}
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.x += 0.025f;
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.x += 0.025f;
		}
	}
	if (GetAsyncKeyState(VK_UP))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.y += 0.025f;		
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.y += 0.025f;
		}
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.y -= 0.025f;
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.y -= 0.025f;
		}
	}
	if (GetAsyncKeyState(VK_DIVIDE))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.z += 0.025f;
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.z += 0.025f;
		}
	}
	if (GetAsyncKeyState(VK_MULTIPLY))
	{
		if (SwitchLight == 1)
		{
			Lights[1].LightPos.z -= 0.025f;
		}
		else if (SwitchLight == 2)
		{
			Lights[2].LightPos.z -= 0.025f;
		}
	}
	if (GetAsyncKeyState('3'))
	{
		Lights[2].LightDirection.x += 0.0025f;
	}
	if (GetAsyncKeyState('4'))
	{
		Lights[2].LightDirection.x -= 0.0025f;
	}
	if (GetAsyncKeyState('5'))
	{
		Lights[2].LightDirection.z += 0.0025f;
	}
	if (GetAsyncKeyState('6'))
	{
		Lights[2].LightDirection.z -= 0.0025f;
	}
	if (GetAsyncKeyState('7'))
	{
		Lights[2].LightDirection.y += 0.0025f;
	}
	if (GetAsyncKeyState('8'))
	{
		Lights[2].LightDirection.y -= 0.0025f;
	}
	//-------------------------------------------------------------------------------------------------------------//
	//Update the Camera
	//-------------------------------------------------------------------------------------------------------------//
	g_View = UpdateViewMatrix();

	//-------------------------------------------------------------------------------------------------------------//
	// Clear the back buffer
	// Clear the depth buffer to 1.0 (max depth)
	//-------------------------------------------------------------------------------------------------------------//
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//add viewport
	//need to update width and height upon resize for it to work properly
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f);
	//-------------------------------------------------------------------------------------------------------------//
	//Render Models
	//-------------------------------------------------------------------------------------------------------------//
	for (size_t i = 0; i < Models.size(); i++)
	{
		RenderModel(Models[i]);
	}
	//-------------------------------------------------------------------------------------------------------------//
	//Render SkyBox Last to not draw certain pixels twice
	//-------------------------------------------------------------------------------------------------------------//
	RenderSkyBox(Sky);
	g_pSwapChain->Present(0, 0);
}

//Render a Model
void RenderModel(Model mod) 
{
	for (size_t j = 0; j < mod.parts.size(); j++)
	{
		//Fill out Constant Buffer/s
		ConstantBuffer cb1;
		ConstantBuffer2 cb2;
		g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);
		cb1.mWorld = XMMatrixTranspose(mod.Model_WorldM * mod.parts[j].Part_WorldMat);
		cb1.mView = XMMatrixTranspose(g_View);
		cb1.mProjection = XMMatrixTranspose(g_Projection);
		for (size_t i = 0; i < Lights.size(); i++)
		{
			//Handle all Lights and Pass information to the Constant buffers
			cb2.vLightDir[i] = Lights[i].LightDirection;
			cb2.vLightColor[i] = Lights[i].LightColor;
			cb2.vLightPos[i] = Lights[i].LightPos;
			cb2.vLightType[i].x = Lights[i].LightType;
			cb2.vLightRange[i].x = Lights[i].LightRange;
			cb2.vInnerAngle[i].x = Lights[i].InnerAngle;
			cb2.vOuterAngle[i].x = Lights[i].OuterAngle;
		}
		cb2.vOutputColor = XMFLOAT4(0, 0, 0, 0);
		//Update Subresources(Update Constant Buffers with the new information)
		g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);
		g_pImmediateContext->UpdateSubresource(g_pConstantBuffer2, 0, nullptr, &cb2, 0, 0);
		//SetVertex Buffer and Index Buffer
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &mod.parts[j].VertexBuffer, &stride, &offset);
		g_pImmediateContext->IASetIndexBuffer(mod.parts[j].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//Set Shaders, Samplers, and Constant Buffers
		g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
		g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
		g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pConstantBuffer2);
		g_pImmediateContext->PSSetShaderResources(0, 1, &mod.parts[j].texture);
		g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
		//Draw Models
		g_pImmediateContext->DrawIndexed(mod.parts[j].IndiceCount, 0, 0);
	}
}

//Used to read from a file
void list_dir(const char *path) 
{
	struct dirent *entry;
	DIR *dir = opendir(path);

	if (dir == NULL) {
		return;
	}
	while ((entry = readdir(dir)) != NULL) 
	{
		objects.push_back(entry->d_name);
	}
	objects.erase(objects.begin());
	objects.erase(objects.begin());
	closedir(dir);
}

//Update our Camera
XMMATRIX UpdateViewMatrix()
{
	//Update eye look
	POINT CursorPos;
	GetCursorPos(&CursorPos);
	if (CursorLock == true)
	{
		pitch += (CursorPos.y - DefaultCursorPosY) * 0.0025f;
		yaw += (CursorPos.x - DefaultCursorPosX) * 0.0025f;
		if (pitch >= 1.5f)
		{
			pitch = 1.5f;
		}
		else if (pitch <= -1.5f)
		{
			pitch = -1.5f;
		}
	}
	XMMATRIX camRotationMatrix;
	XMMATRIX groundWorld;
	float speed = 0.2f;
	if (GetAsyncKeyState('A'))
	{
		moveLeftRight -= speed;
	}
	if (GetAsyncKeyState('D'))
	{
		moveLeftRight += speed;
	}
	if (GetAsyncKeyState('W'))
	{
		moveBackForward += speed;
	}
	if (GetAsyncKeyState('S'))
	{
		moveBackForward -= speed;
	}
	if (GetAsyncKeyState(VK_SPACE))
	{
		moveUpDown += speed;
	}
	if (GetAsyncKeyState(VK_LSHIFT))
	{
		moveUpDown -= speed;
	}
	if ((mouseCurrState.lX != mouseLastState.lX) || (mouseCurrState.lY != mouseLastState.lY))
	{
		yaw += mouseLastState.lX * 0.001f;

		pitch += mouseCurrState.lY * 0.001f;

		mouseLastState = mouseCurrState;
	}	
	camRotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, 0);
	camTarget = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camTarget = XMVector3Normalize(camTarget);
	XMMATRIX RotateYTempMatrix;
	RotateYTempMatrix = XMMatrixRotationY(yaw);
	camRight = XMVector3TransformCoord(DefaultRight, RotateYTempMatrix);
	camUp = XMVector3TransformCoord(DefaultUp, RotateYTempMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, RotateYTempMatrix);

	camPosition += moveLeftRight * camRight;
	camPosition += moveBackForward * camForward;
	camPosition += moveUpDown * camUp;
	camTarget = camPosition + camTarget;
	XMMATRIX camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);
	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;
	moveUpDown = 0.0f;
	if (CursorLock == true)
	{
		SetCursorPos(DefaultCursorPosX, DefaultCursorPosY);
		ShowCursor(false);
	}
	if (GetAsyncKeyState(VK_ESCAPE))
	{
		CursorLock = false;
	}
	if (GetAsyncKeyState(VK_BACK))
	{
		CursorLock = true;
		ShowCursor(false);
	}
	return camView;
}

//Create Grid
Part CreateGrid(float width, float depth, UINT n, UINT m, float angleX, float angleY, float angleZ)
{
	int indices = 0;
	Part part;
	part.VertCount = m * n;
	UINT faceCount = (m - 1)*(n - 1) * 2;

	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	
	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	part.verts.resize(part.VertCount);
	for (UINT i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (UINT j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;
			float y = j + i * m;
			//Set Verts
			part.verts[i*n + j].Pos.x = x;
			part.verts[i*n + j].Pos.y = -0.75f;
			part.verts[i*n + j].Pos.z = z;
			//Set Normals
			//part.verts[i*n + j].Pos;
			part.verts[i*n + j].Normal.x = 0.0f;
			part.verts[i*n + j].Normal.y = 1.0f;
			part.verts[i*n + j].Normal.z = 0.0f;
			//Stretch Texture
			part.verts[i*n + j].Tex.x = j * du;
			part.verts[i*n + j].Tex.y = i * dv;
		}
	}

	part.indices.resize(faceCount * 3); 
	UINT k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (UINT j = 0; j < n - 1; ++j)
		{
			part.indices[k] = i * n + j;
			part.indices[k + 1] = i * n + j + 1;
			part.indices[k + 2] = (i + 1)*n + j;

			part.indices[k + 3] = (i + 1)*n + j;
			part.indices[k + 4] = i * n + j + 1;
			part.indices[k + 5] = (i + 1)*n + j + 1;

			k += 6;
		}
	}

	indices = part.indices.size();
	part.IndiceCount = indices;
	part.Part_WorldMat = XMMatrixIdentity() * XMMatrixRotationX(angleX);
	//part.Part_WorldMat *= XMMatrixRotationY(angleY);
	//part.Part_WorldMat *= XMMatrixRotationZ(angleZ);
	return part;
}

//Create Sphere
Part CreateSphere(int LatLines, int LongLines, float Scale)
{	
	Part Sphere;
	Sphere.VertCount = ((LatLines - 2) * LongLines) + 2;
	int NumSphereFaces = ((LatLines - 3)*(LongLines) * 2) + (LongLines * 2);

	float sphereYaw = 0.0f;
	float spherePitch = 0.0f;
	float du = 1.0f / ((float)LatLines - 1.0f);
	float dv = 1.0f / ((float)LongLines - 1.0f);

	Sphere.verts = std::vector<SimpleVertex>(Sphere.VertCount);

	XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	Sphere.verts[0].Pos.x = 0.0f;
	Sphere.verts[0].Pos.y = 0.0f;
	Sphere.verts[0].Pos.z = 1.0f;
	XMMATRIX RotationX;
	XMMATRIX RotationY;
	for (DWORD i = 0; i < LatLines - 2; ++i)
	{
		spherePitch = (i + 1) * (3.14 / (LatLines - 1));
		RotationX = XMMatrixRotationX(spherePitch);
		for (DWORD j = 0; j < LongLines; ++j)
		{
			sphereYaw = j * (6.28 / (LongLines));
			RotationY = XMMatrixRotationZ(sphereYaw);
			currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (RotationX * RotationY));
			currVertPos = XMVector3Normalize(currVertPos);
			Sphere.verts[i*LongLines + j + 1].Pos.x = -XMVectorGetX(currVertPos);
			Sphere.verts[i*LongLines + j + 1].Pos.y = -XMVectorGetY(currVertPos);
			Sphere.verts[i*LongLines + j + 1].Pos.z = -XMVectorGetZ(currVertPos);

			Sphere.verts[i*LongLines + j].Tex.x = j * du;
			Sphere.verts[i*LongLines + j].Tex.y = i * dv;
			
			Sphere.verts[i*LongLines + j].Normal.x = Sphere.verts[i*LongLines + j + 1].Pos.x * 0.01f;
			Sphere.verts[i*LongLines + j].Normal.y = Sphere.verts[i*LongLines + j + 1].Pos.y * 0.01f;
			Sphere.verts[i*LongLines + j].Normal.z = Sphere.verts[i*LongLines + j + 1].Pos.z * 0.01f;
		}
	}

	Sphere.verts[Sphere.VertCount - 1].Pos.x = 0.0f;
	Sphere.verts[Sphere.VertCount - 1].Pos.y = 0.0f;
	Sphere.verts[Sphere.VertCount - 1].Pos.z = -1.0f;
	SimpleVertex Trade;
	//for (size_t i = 0; i < Sphere.verts.size() - 2; i++)
	//{
	//	if (i == 0 || i % 2 == 0)
	//	{
	//		Trade = Sphere.verts[i];
	//		Sphere.verts[i] = Sphere.verts[i + 2];
	//		Sphere.verts[i + 2] = Trade;
	//	}
	//	//i += 4;
	//}

	Sphere.indices = std::vector<UINT>(NumSphereFaces * 3);
	Sphere.IndiceCount = NumSphereFaces * 3;
	int k = 0;
	for (UINT l = 0; l < LongLines - 1; ++l)
	{
		Sphere.indices[k] = 0;
		Sphere.indices[k + 1] = l + 1;
		Sphere.indices[k + 2] = l + 2;
		k += 3;
	}

	Sphere.indices[k] = 0;
	Sphere.indices[k + 1] = LongLines;
	Sphere.indices[k + 2] = 1;
	k += 3;

	for (UINT i = 0; i < LatLines - 3; ++i)
	{
		for (UINT j = 0; j < LongLines - 1; ++j)
		{
			Sphere.indices[k] = i * LongLines + j + 1;
			Sphere.indices[k + 1] = i * LongLines + j + 2;
			Sphere.indices[k + 2] = (i + 1)*LongLines + j + 1;

			Sphere.indices[k + 3] = (i + 1)*LongLines + j + 1;
			Sphere.indices[k + 4] = i * LongLines + j + 2;
			Sphere.indices[k + 5] = (i + 1)*LongLines + j + 2;

			k += 6; // next quad
		}

		Sphere.indices[k] = (i*LongLines) + LongLines;
		Sphere.indices[k + 1] = (i*LongLines) + 1;
		Sphere.indices[k + 2] = ((i + 1)*LongLines) + LongLines;

		Sphere.indices[k + 3] = ((i + 1)*LongLines) + LongLines;
		Sphere.indices[k + 4] = (i*LongLines) + 1;
		Sphere.indices[k + 5] = ((i + 1)*LongLines) + 1;

		k += 6;
	}

	for (UINT l = 0; l < LongLines - 1; ++l)
	{
		Sphere.indices[k] = Sphere.VertCount - 1;
		Sphere.indices[k + 1] = (Sphere.VertCount - 1) - (l + 1);
		Sphere.indices[k + 2] = (Sphere.VertCount - 1) - (l + 2);
		k += 3;
	}

	Sphere.indices[k] = Sphere.VertCount - 1;
	Sphere.indices[k + 1] = (Sphere.VertCount - 1) - LongLines;
	Sphere.indices[k + 2] = (Sphere.VertCount - 2);
	for (UINT i = 0; i < Sphere.VertCount; i++)
	{
		Sphere.verts[i].Pos.x *= Scale;
		Sphere.verts[i].Pos.y *= Scale;
		Sphere.verts[i].Pos.z *= Scale;
	}
	//UINT tm;
	//for (size_t i = 0; i < Sphere.indices.size() - 2; i++)
	//{
	//	if (i == 0 || i % 2 == 0)
	//	{
	//		tm = Sphere.indices[i];
	//		Sphere.indices[i] = Sphere.indices[i + 2];
	//		Sphere.indices[i + 2] = tm;
	//	}
	//	//i += 4;
	//}
	//Need to Create Buffers
	//XMMATRIX temp = XMMatrixIdentity();
	//SkySphere.World_M = XMVector3TransformCoord({ 0.0f, 0.0f, 0.0f, 1.0f }, temp);
	/*temp.r[3].m128_f32[0] = 0.0f;
	temp.r[3].m128_f32[1] = 0.0f;
	temp.r[3].m128_f32[2] = 0.0f;*/
	/*Sphere.World_M *= XMMatrixRotationX(45.0f);
	Sphere.World_M *= XMMatrixRotationY(0.0f);
	Sphere.World_M *= XMMatrixRotationZ(0.0f);*/
	return Sphere;
}

//Render our SkyBox
void RenderSkyBox(Model SkySphere) 
{
	ConstantBuffer cb1;
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 1000.0f);
	cb1.mWorld = XMMatrixTranspose(SkySphere.Model_WorldM * XMMatrixTranslationFromVector(camPosition));
	cb1.mView = XMMatrixTranspose(g_View);
	cb1.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cb1, 0, 0);

	//SetVertex Buffers
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &SkySphere.parts[0].VertexBuffer, &stride, &offset);
	g_pImmediateContext->IASetIndexBuffer(SkySphere.parts[0].IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_pImmediateContext->VSSetShader(g_pCubeMapVS, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
	g_pImmediateContext->PSSetShader(g_pCubeMapPS, nullptr, 0);
	
	g_pImmediateContext->PSSetShaderResources(1, 1, &SkySphere.parts[0].texture);
	g_pImmediateContext->PSSetSamplers(1, 1, &g_pSamplerTrilinear);
	g_pImmediateContext->DrawIndexed(SkySphere.parts[0].IndiceCount, 0, 0);
}


