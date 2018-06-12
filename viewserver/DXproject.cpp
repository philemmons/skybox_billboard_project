

#include "ground.h"
//	defines
#define MAX_LOADSTRING 1000
#define TIMER1 111
//	global variables
HINSTANCE hInst;											//	program number = instance
TCHAR szTitle[MAX_LOADSTRING];								//	name in window title
TCHAR szWindowClass[MAX_LOADSTRING];						//	class name of window
HWND hMain = NULL;											//	number of windows = handle window = hwnd
static char MainWin[] = "MainWin";							//	class name
HBRUSH  hWinCol = CreateSolidBrush(RGB(180, 180, 180));		//	a color
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

//	+++++++++++++++++++++++++++++++++++++++++++++++++
//	DIRECTX stuff follows here
//
//	first the global variables (DirectX Objects)
//
//	second the initdevice(..) function where we load all DX stuff
//
//	third the Render() function, the equivalent to the OnPaint(), but not from Windows, but from DirectX
//
//	lets go:

// GLOBALS:

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;

// device context
ID3D11Device*           g_pd3dDevice = NULL;			// is for initialization and loading things (pictures, models, ...) <- InitDevice()
ID3D11DeviceContext*    g_pImmediateContext = NULL;		// is for render your models w/ pics on the screen					<- Render()
														// page flipping:
IDXGISwapChain*         g_pSwapChain = NULL;
//screen <- thats our render target
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
//how a vertex looks like
ID3D11InputLayout*      g_pVertexLayout = NULL;
//our model (array of vertices on the GPU MEM)
ID3D11Buffer*           g_pVertexBuffer = NULL; //our square

ID3D11Buffer*           g_pVertexBufferStar = NULL; //our rectangle for the shooting star 

ID3D11Buffer*           g_pVertexBufferDS = NULL; //our death star
int						g_vertices_ds;

ID3D11Buffer*           g_pVertexBufferSUN = NULL; //our sun
int						g_vertices_sun;

ID3D11Buffer*           g_pVertexBufferSky = NULL; //our star system
int						g_vertices_sky;

//exchange of data, e.g. sending mouse coordinates to the GPU
ID3D11Buffer*			g_pConstantBuffer11 = NULL;

// function on the GPU what to do with the model exactly
ID3D11VertexShader*     g_pVertexShader = NULL;
ID3D11VertexShader*     g_pVertexShaderSun = NULL;

ID3D11PixelShader*      g_pPixelShader = NULL;
ID3D11PixelShader*      g_pPixelShaderSky = NULL;

//depth stencli buffer
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;

//for transparency:
ID3D11BlendState*					g_BlendState;

//Texture
ID3D11ShaderResourceView*           g_Texture = NULL;
ID3D11ShaderResourceView*           g_TextureDS_NORM = NULL; //not used yet
ID3D11ShaderResourceView*           g_textureDS = NULL;
ID3D11ShaderResourceView*           g_textureSUN = NULL;

//new
ID3D11ShaderResourceView*           g_TextureP1 = NULL;
ID3D11ShaderResourceView*           g_TextureP2 = NULL;
ID3D11ShaderResourceView*           g_TextureP3 = NULL;
ID3D11ShaderResourceView*           g_TextureP4 = NULL;
ID3D11ShaderResourceView*           g_TextureP5 = NULL;
ID3D11ShaderResourceView*           g_TextureP6 = NULL;
ID3D11ShaderResourceView*           g_TextureE = NULL;
ID3D11ShaderResourceView*           g_TextureMF = NULL;
ID3D11ShaderResourceView*           g_TextureTF = NULL;
ID3D11ShaderResourceView*           g_TextureSS = NULL;
ID3D11ShaderResourceView*           g_TextureBH = NULL;
ID3D11ShaderResourceView*           g_TextureA1 = NULL;
ID3D11ShaderResourceView*           g_TextureS1 = NULL;

//Texture Sampler
ID3D11SamplerState*                 g_Sampler = NULL;

//rasterizer states			//clockwise		//counter clockwise		//				//wireframe - triangle pixel lines
ID3D11RasterizerState				*rs_CW, *rs_CCW, *rs_NO, *rs_Wire;

//depth state
ID3D11DepthStencilState				*ds_on, *ds_off;

//=====		TERRAIN
ID3D11Buffer*			terrainBuffer = NULL;
int						terrianVertCount = 0;
ID3D11VertexShader*		terrainVertexShader = NULL;
ID3D11PixelShader*		terrainPixelShader = NULL;

ID3D11ShaderResourceView*	terrainTexture = NULL;
ID3D11ShaderResourceView*	rockTexture = NULL;


//=====		WATER
ID3D11Buffer*			waterBuffer = NULL;
int						waterVertCount = 0;
ID3D11VertexShader*		waterVertexShader = NULL;
ID3D11PixelShader*		waterPixelShader = NULL;

ID3D11ShaderResourceView*   waveOneTexture = NULL;
ID3D11ShaderResourceView*   waveTwoTexture = NULL;



//	structures we need later
XMMATRIX g_world;//model: per object position and rotation and scaling of the object
XMMATRIX g_view;//camera: position and rotation of the camera
XMMATRIX g_projection;//perspective: angle of view, near plane / far plane

camera cam;

struct VS_CONSTANT_BUFFER
	{
	float texOneOffsetX;
	float texOneOffsetY;

	XMMATRIX world;//model: per object position and rotation and scaling of the object
	XMMATRIX view;//camera: position and rotation of the camera
	XMMATRIX projection;//perspective: angle of view, near plane / far plane

	XMFLOAT4 campos;//position of the camera for various effects

	XMFLOAT4 offsets;// camera perspectives

	XMFLOAT4 waveOffset;

	};	//we will copy that periodically to its twin on the GPU, with some_variable_a/b for the mouse coordinates
		//note: we can only copy chunks of 16 byte to the GPU
VS_CONSTANT_BUFFER VsConstData;		//gloab object of this structure

struct PosTexVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};
struct PosTexNormVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
	XMFLOAT3 Norm;
};

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
	{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hMain, &rc);	//getting the windows size into a RECT structure
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

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
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
		{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hMain;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
		}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;



	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader


	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShader", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
		{
		pVSBlob->Release();
		return hr;
		}

	//Compile the vertex shader for height mapping - sun
	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShaderSun", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the height mapping shader - sun
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShaderSun);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	////=====		COMPILE VS TERRAIN
	//hr = CompileShaderFromFile(L"shader.fx", "vs_terrain", "vs_4_0", &pVSBlob);
	//if (FAILED(hr))
	//{
	//	MessageBox(NULL,
	//		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
	//	return hr;
	//}

	////=====		CREATE VS TERRAIN
	//hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &terrainVertexShader);
	//if (FAILED(hr))
	//{
	//	pVSBlob->Release();
	//	return hr;
	//}
	


	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
		{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader number 1
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSsky", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the pixel shader number 2
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderSky);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	////=====		COMPILE PS TERRAIN
	//hr = CompileShaderFromFile(L"shader.fx", "ps_terrain", "ps_4_0", &pPSBlob);
	//if (FAILED(hr))
	//{
	//	MessageBox(NULL,
	//		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
	//	return hr;
	//}

	////=====		CREATE PS TERRAIN
	//hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &terrainPixelShader);
	//pPSBlob->Release();
	//if (FAILED(hr))
	//	return hr;

	//new
	SimpleVertex vertices[36];
	float texWide = (1.0f / 4.0f);
	float texHeight = (1.0f / 3.0f);
	//front
	vertices[0].Pos = XMFLOAT3(-1, 1, 1);		vertices[0].Tex = XMFLOAT2(1 * texWide, 1 * texHeight);
	vertices[1].Pos = XMFLOAT3(-1, -1, 1);		vertices[1].Tex = XMFLOAT2(1 * texWide, 2 * texHeight);
	vertices[2].Pos = XMFLOAT3(1, -1, 1);		vertices[2].Tex = XMFLOAT2(2 * texWide, 2 * texHeight);

	vertices[3].Pos = XMFLOAT3(1, -1, 1);		vertices[3].Tex = XMFLOAT2(2 * texWide, 2 * texHeight);
	vertices[4].Pos = XMFLOAT3(1, 1, 1);		vertices[4].Tex = XMFLOAT2(2 * texWide, 1 * texHeight);
	vertices[5].Pos = XMFLOAT3(-1, 1, 1);		vertices[5].Tex = XMFLOAT2(1 * texWide, 1 * texHeight);
	//left
	vertices[6].Pos = XMFLOAT3(-1, 1, -1);		vertices[6].Tex = XMFLOAT2(0 * texWide, 1 * texHeight);
	vertices[7].Pos = XMFLOAT3(-1, -1, -1);		vertices[7].Tex = XMFLOAT2(0 * texWide, 2 * texHeight);
	vertices[8].Pos = XMFLOAT3(-1, -1, 1);		vertices[8].Tex = XMFLOAT2(1 * texWide, 2 * texHeight);

	vertices[9].Pos = XMFLOAT3(-1, -1, 1);		vertices[9].Tex = XMFLOAT2(1 * texWide, 2 * texHeight);
	vertices[10].Pos = XMFLOAT3(-1, 1, 1);		vertices[10].Tex = XMFLOAT2(1 * texWide, 1 * texHeight);
	vertices[11].Pos = XMFLOAT3(-1, 1, -1);		vertices[11].Tex = XMFLOAT2(0 * texWide, 1 * texHeight);
	//back
	vertices[12].Pos = XMFLOAT3(1, 1, -1);		vertices[12].Tex = XMFLOAT2(3 * texWide, 1 * texHeight);
	vertices[13].Pos = XMFLOAT3(1, -1, -1);		vertices[13].Tex = XMFLOAT2(3 * texWide, 2 * texHeight);
	vertices[14].Pos = XMFLOAT3(-1, -1, -1);	vertices[14].Tex = XMFLOAT2(4 * texWide, 2 * texHeight);

	vertices[15].Pos = XMFLOAT3(-1, -1, -1);	vertices[15].Tex = XMFLOAT2(4 * texWide, 2 * texHeight);
	vertices[16].Pos = XMFLOAT3(-1, 1, -1);		vertices[16].Tex = XMFLOAT2(4 * texWide, 1 * texHeight);
	vertices[17].Pos = XMFLOAT3(1, 1, -1);		vertices[17].Tex = XMFLOAT2(3 * texWide, 1 * texHeight);
	//right
	vertices[18].Pos = XMFLOAT3(1, -1, 1);		vertices[18].Tex = XMFLOAT2(2 * texWide, 2 * texHeight);
	vertices[19].Pos = XMFLOAT3(1, -1, -1);		vertices[19].Tex = XMFLOAT2(3 * texWide, 2 * texHeight);
	vertices[20].Pos = XMFLOAT3(1, 1, -1);		vertices[20].Tex = XMFLOAT2(3 * texWide, 1 * texHeight);

	vertices[21].Pos = XMFLOAT3(1, 1, -1);		vertices[21].Tex = XMFLOAT2(3 * texWide, 1 * texHeight);
	vertices[22].Pos = XMFLOAT3(1, 1, 1);		vertices[22].Tex = XMFLOAT2(2 * texWide, 1 * texHeight);
	vertices[23].Pos = XMFLOAT3(1, -1, 1);		vertices[23].Tex = XMFLOAT2(2 * texWide, 2 * texHeight);
	////top
	vertices[24].Pos = XMFLOAT3(-1, 1, 1);		vertices[24].Tex = XMFLOAT2(1 * texWide, 1 * texHeight);
	vertices[25].Pos = XMFLOAT3(1, 1, 1);		vertices[25].Tex = XMFLOAT2(2 * texWide, 1 * texHeight);
	vertices[26].Pos = XMFLOAT3(1, 1, -1);		vertices[26].Tex = XMFLOAT2(2 * texWide, 0 * texHeight);

	vertices[27].Pos = XMFLOAT3(1, 1, -1);		vertices[27].Tex = XMFLOAT2(2 * texWide, 0 * texHeight);
	vertices[28].Pos = XMFLOAT3(-1, 1, -1);		vertices[28].Tex = XMFLOAT2(1 * texWide, 0 * texHeight);
	vertices[29].Pos = XMFLOAT3(-1, 1, 1);		vertices[29].Tex = XMFLOAT2(1 * texWide, 1 * texHeight);
	//bottom
	vertices[30].Pos = XMFLOAT3(-1, -1, 1);		vertices[30].Tex = XMFLOAT2(1 * texWide, 2 * texHeight);
	vertices[31].Pos = XMFLOAT3(-1, -1, -1);	vertices[31].Tex = XMFLOAT2(1 * texWide, 3 * texHeight);
	vertices[32].Pos = XMFLOAT3(1, -1, -1);		vertices[32].Tex = XMFLOAT2(2 * texWide, 3 * texHeight);

	vertices[33].Pos = XMFLOAT3(1, -1, -1);		vertices[33].Tex = XMFLOAT2(2 * texWide, 3 * texHeight);
	vertices[34].Pos = XMFLOAT3(1, -1, 1);		vertices[34].Tex = XMFLOAT2(2 * texWide, 2 * texHeight);
	vertices[35].Pos = XMFLOAT3(-1, -1, 1);		vertices[35].Tex = XMFLOAT2(1 * texWide, 2 * texHeight);

	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferSky);
	if (FAILED(hr))
		return hr;

	// Create vertex buffer, the triangle
	SimpleVertex bb_vertices[6];
	bb_vertices[0].Pos = XMFLOAT3(-1, 1, 1);	//left top
	bb_vertices[1].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	bb_vertices[2].Pos = XMFLOAT3(-1, -1, 1); //left bottom
	bb_vertices[0].Tex = XMFLOAT2(0.0f, 0.0f);
	bb_vertices[1].Tex = XMFLOAT2(1.0f, 1.0f);
	bb_vertices[2].Tex = XMFLOAT2(0.0f, 1.0f);

	bb_vertices[3].Pos = XMFLOAT3(-1, 1, 1);	//left top
	bb_vertices[4].Pos = XMFLOAT3(1, 1, 1);	//right top
	bb_vertices[5].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	bb_vertices[3].Tex = XMFLOAT2(0.0f, 0.0f);			//left top
	bb_vertices[4].Tex = XMFLOAT2(1.0f, 0.0f);			//right top
	bb_vertices[5].Tex = XMFLOAT2(1.0f, 1.0f);			//right bottom


	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = bb_vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Create vertex buffer, the triangle
	SimpleVertex ss_vertices[6];
	ss_vertices[0].Pos = XMFLOAT3(-1, 0.1875f, 1);	//left top
	ss_vertices[1].Pos = XMFLOAT3(1, -0.1875f, 1);	//right bottom
	ss_vertices[2].Pos = XMFLOAT3(-1, -0.1875f, 1); //left bottom
	ss_vertices[0].Tex = XMFLOAT2(0.0f, 0.0f);
	ss_vertices[1].Tex = XMFLOAT2(1.0f, 1.0f);
	ss_vertices[2].Tex = XMFLOAT2(0.0f, 1.0f);

	ss_vertices[3].Pos = XMFLOAT3(-1, 0.1875f, 1);	//left top
	ss_vertices[4].Pos = XMFLOAT3(1, 0.1875f, 1);	//right top
	ss_vertices[5].Pos = XMFLOAT3(1, -0.1875f, 1);	//right bottom
	ss_vertices[3].Tex = XMFLOAT2(0.0f, 0.0f);			//left top
	ss_vertices[4].Tex = XMFLOAT2(1.0f, 0.0f);			//right top
	ss_vertices[5].Tex = XMFLOAT2(1.0f, 1.0f);			//right bottom


	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = ss_vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferStar);
	if (FAILED(hr))
		return hr;

	//=====		MESH VARIABLES

	int meshX = 500,
		meshZ = 500,
		fraction = 500;

	//=====		TERRAIN MESH

	terrianVertCount = meshX * meshZ * 6;

	PosTexVertex* terrainData = new (nothrow) PosTexVertex[terrianVertCount];
	if (terrainData == nullptr) {
		//error assigning memory. Take measures.
	}

	float step = 1.0f / fraction;

	for (int jj = 0; jj < meshZ; ++jj)
	{
		for (int ii = 0; ii < meshX; ++ii)
		{
			unsigned int offset = (jj * 500 + ii) * 6;
			XMFLOAT3 squareOffset = XMFLOAT3(ii, 0, jj);
			XMFLOAT2 texOffset = XMFLOAT2(step * ii, step * jj);

			//triangle 1
			terrainData[offset + 0].Pos = XMFLOAT3(0, 0, 0) + squareOffset;
			terrainData[offset + 1].Pos = XMFLOAT3(0, 0, 1) + squareOffset;
			terrainData[offset + 2].Pos = XMFLOAT3(1, 0, 0) + squareOffset;

			terrainData[offset + 0].Tex = (XMFLOAT2(0, 0) + texOffset);
			terrainData[offset + 1].Tex = (XMFLOAT2(0, step) + texOffset);
			terrainData[offset + 2].Tex = (XMFLOAT2(step, 0) + texOffset);

			//triangle 2
			terrainData[offset + 3].Pos = XMFLOAT3(1, 0, 0) + squareOffset;
			terrainData[offset + 4].Pos = XMFLOAT3(0, 0, 1) + squareOffset;
			terrainData[offset + 5].Pos = XMFLOAT3(1, 0, 1) + squareOffset;

			terrainData[offset + 3].Tex = (XMFLOAT2(step, 0) + texOffset);
			terrainData[offset + 4].Tex = (XMFLOAT2(0, step) + texOffset);
			terrainData[offset + 5].Tex = (XMFLOAT2(step, step) + texOffset);
		}
	}

	//=====		DESCRIBE/DEFINE/CREATE TERRAINBUFFER

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(PosTexVertex) * terrianVertCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = terrainData;

	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &terrainBuffer);
	if (FAILED(hr))
		return hr;

	delete[] terrainData;


	// Supply the vertex shader constant data
	VsConstData.texOneOffsetX = 0;
	VsConstData.texOneOffsetY = 0;


	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &VsConstData;

	// Create the buffer.
	hr = g_pd3dDevice->CreateBuffer(&cbDesc, &InitData, &g_pConstantBuffer11);
	if (FAILED(hr))
		return hr;


	//============================	Init textures
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"space.png", NULL, NULL, &g_Texture, NULL);
	if (FAILED(hr))
		return hr;

	//deathstar unused
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"ds_norm.png", NULL, NULL, &g_TextureDS_NORM, NULL);
	if (FAILED(hr))
		return hr;
	//deathstar 
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"ds_S.png", NULL, NULL, &g_textureDS, NULL);
	if (FAILED(hr))
		return hr;
	//sun
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"sun.jpg", NULL, NULL, &g_textureSUN, NULL);
	if (FAILED(hr))
		return hr;
	//===========================	billboards textures
	//planet 1-6
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p1.png", NULL, NULL, &g_TextureP1, NULL);
	if (FAILED(hr))
		return hr;
	
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p2.png", NULL, NULL, &g_TextureP2, NULL);
	if (FAILED(hr))
		return hr;
	
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p3.png", NULL, NULL, &g_TextureP3, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p4.png", NULL, NULL, &g_TextureP4, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p5.png", NULL, NULL, &g_TextureP5, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"p6.png", NULL, NULL, &g_TextureP6, NULL);
	if (FAILED(hr))
		return hr;
	//enterprise
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"e.png", NULL, NULL, &g_TextureE, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"mf.png", NULL, NULL, &g_TextureMF, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"tf.png", NULL, NULL, &g_TextureTF, NULL);
	if (FAILED(hr))
		return hr;
	//shooting star
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"ss.png", NULL, NULL, &g_TextureSS, NULL);
	if (FAILED(hr))
		return hr;
	//black hole
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"bh.png", NULL, NULL, &g_TextureBH, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"a1.png", NULL, NULL, &g_TextureA1, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"S1.png", NULL, NULL, &g_TextureS1, NULL);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"heightmap.png", 0, 0, &terrainTexture, 0);
	if (FAILED(hr))
		return hr;

	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"grass.jpg", 0, 0, &rockTexture, 0);
	if (FAILED(hr))
		return hr;

	//===================	sampler init
	D3D11_SAMPLER_DESC sampDesc;

	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_Sampler);
	if (FAILED(hr))
		return hr;
	//==================== create Texture Sampler
	g_pImmediateContext->PSSetSamplers(0, 1, &g_Sampler);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_Sampler);

	//===================	blendstate init

	//blendstate - unused
	D3D11_BLEND_DESC blendStateDesc;

	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);

	//===========================	matrices
	//===========================	world
	g_world = XMMatrixIdentity();

	//===========================	view:
	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);//camera position
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);//look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_view = XMMatrixLookAtLH(Eye, At, Up);

	//===========================	perspective:
	// Initialize the projection matrix
	g_projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 10000.0f);

	
	
	//===========================	load the death star and the sun 
	LoadOBJ("ds.obj", g_pd3dDevice, &g_pVertexBufferDS, &g_vertices_ds);
	Load3DS("sphere.3ds", g_pd3dDevice, &g_pVertexBufferSUN, &g_vertices_sun, FALSE);
	

	//depth buffer:
	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;

	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;


	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;

	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	//		*****************************************		rasterizer states:
	//setting the rasterizer - so we can check it later
	D3D11_RASTERIZER_DESC			RS_CW, RS_Wire;

	RS_CW.AntialiasedLineEnable = FALSE;
	RS_CW.CullMode = D3D11_CULL_BACK;
	RS_CW.DepthBias = 0;
	RS_CW.DepthBiasClamp = 0.0f;
	RS_CW.DepthClipEnable = true;
	RS_CW.FillMode = D3D11_FILL_SOLID;
	RS_CW.FrontCounterClockwise = false;
	RS_CW.MultisampleEnable = FALSE;
	RS_CW.ScissorEnable = false;
	RS_CW.SlopeScaledDepthBias = 0.0f;

	//rasterizer state clockwise triangles
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CW);//display front

	//rasterizer state counterclockwise triangles
	RS_CW.CullMode = D3D11_CULL_FRONT;
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CCW);//display back
	RS_Wire = RS_CW;
	RS_Wire.CullMode = D3D11_CULL_NONE;

	//rasterizer state seeing both sides of the triangle
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_NO);

	//rasterizer state wirefrime
	RS_Wire.FillMode = D3D11_FILL_WIREFRAME;
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_Wire);

	//init depth stats:
	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state ON AND OFF
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);

	return S_OK;
	}

//--------------------------------------------------------------------------------------
// Render function
//--------------------------------------------------------------------------------------
	static bool boost = false;
	static float accel = 0;

	float plane_y_angle = 0;
	bool plane_y_positive = false;
	bool plane_y_negative = false;

	float plane_x_angle = 0;
	bool plane_x_positive = false;
	bool plane_x_negative = false;

	float plane_z_angle = 0;
	bool plane_z_positive = false;
	bool plane_z_negative = false;

	static float texOneOffsetX = 0.0f, texOneOffsetY = 0.0f;


void Render()
	{

	texOneOffsetX += tan(0.000001);
	texOneOffsetY += 0;


	// Set stuff for all objects
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	// Clear the back buffer 
	float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // red,green,blue,alpha
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// clear the blender - used with two textures on earth from day to night
	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);


	// Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderSky, NULL, 0);

	
	
	g_pImmediateContext->RSSetState(rs_CW);

	XMMATRIX Mobject;
	
	//change the world matrix

	XMMATRIX V = g_view;
	V = cam.calculate_view(V);
	//VsConstData.projection = g_projection;

	//			************			render the skybox:				********************
	//a neg cam.pos.y will position the camera outside the skybox???
	XMMATRIX Tv = XMMatrixTranslation(cam.pos.x, cam.pos.y, -cam.pos.z);
	
	XMMATRIX Rx = XMMatrixIdentity();
	XMMATRIX S = XMMatrixScaling(0.5, 0.5, 0.5);
	XMMATRIX Msky = S*Rx*Tv; //from left to right

	VsConstData.world = Msky;
	VsConstData.view = V;
	VsConstData.projection = g_projection;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);	//setting it enable for the PixelShader
	
																			// Set vertex buffer, setting the model
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferSky, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_Texture);

	g_pImmediateContext->RSSetState(rs_CCW);//to see it from the inside
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);//no depth writing

	g_pImmediateContext->Draw(36, 0);		//render skybox

	g_pImmediateContext->RSSetState(rs_CW);//reset to default
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);//reset to default

	
	//			************			render the DS:				********************

	if (boost) {
		accel += 0.000001;
	}
	else
		accel = 0.00008;

	if (plane_y_positive && !plane_y_negative)
	{
		plane_y_angle += accel;
	}
	else if (plane_y_negative && !plane_y_positive)
	{
		plane_y_angle -= accel;
	}

	if (plane_x_positive && !plane_x_negative)
	{
		plane_x_angle += accel;
	}
	else if (plane_x_negative && !plane_x_positive)
	{
		plane_x_angle -= accel;
	}

	if (plane_z_positive && !plane_z_negative)
	{
		plane_z_angle += accel;
	}
	else if (plane_z_negative && !plane_z_positive)
	{
		plane_z_angle -= accel;
	}

	XMMATRIX T3 = XMMatrixTranslation(plane_x_angle, plane_y_angle, plane_z_angle);


	XMMATRIX T = XMMatrixTranslation(5, 3, 20);
	//XMMATRIX Rz = XMMatrixIdentity();
	XMMATRIX Rz = XMMatrixRotationZ( XM_PIDIV4 + XM_PIDIV2/6);
	Rx = XMMatrixRotationX(XM_PIDIV2/3);
	//Rx = XMMatrixIdentity();
	static float angle = 0;
	angle = angle + 0.0001;

	XMMATRIX Ry = XMMatrixRotationY(angle/4);
	S = XMMatrixScaling(0.5, 0.5, 0.5);
	Mobject = S*Rz*Rx*Ry*T * T3; //from left to right

	VsConstData.world = Mobject;
	VsConstData.view = V;
	VsConstData.projection = g_projection;
	//?????
	VsConstData.campos.x = cam.pos.x;
	VsConstData.campos.y = cam.pos.y;
	VsConstData.campos.z = cam.pos.z;
	VsConstData.campos.w = 1;

	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferDS, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_textureDS);
	
	g_pImmediateContext->Draw(g_vertices_ds, 0);		//render planet

	//			************			render the SUN:				********************
	T = XMMatrixTranslation(-8, -5, 20);
	Rx = XMMatrixRotationX(XM_PIDIV2);


	Ry = XMMatrixRotationY(-angle);
	S = XMMatrixScaling(0.008, 0.008, 0.008);
	Mobject = S*Rx*Ry*T; //from left to right

	VsConstData.texOneOffsetX = texOneOffsetX;
	VsConstData.texOneOffsetY = texOneOffsetY;

	VsConstData.world = Mobject;

	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferSUN, &stride, &offset);
	
	g_pImmediateContext->VSSetShader(g_pVertexShaderSun, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderSky, NULL, 0);
	
	g_pImmediateContext->VSSetShaderResources(1, 1, &g_textureSUN);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_textureSUN);

	g_pImmediateContext->Draw(g_vertices_sun, 0);		//render planet												


//			************			render billboards:				********************

//idea: pass the inverse rotation view matrix to the world matrix
	XMMATRIX Vc = V;
	Vc._41 = 0;
	Vc._42 = 0;
	Vc._43 = 0;
	XMVECTOR f;
	Vc = XMMatrixInverse(&f, Vc);

	//Rx = XMMatrixRotationX(XM_PIDIV2);
	//Ry = XMMatrixRotationY(XM_PI);

	T = XMMatrixTranslation(-30,5, 20);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP1);
	g_pImmediateContext->Draw(6, 0);		//render rectangle


	T = XMMatrixTranslation(-5, -3, -30);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP2);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(7, 10, -7);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP3);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(30, 8, 20);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP4);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(-10, -15, 25);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP5);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(15, -20, -35);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureP6);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(30, 5, -10);
	S = XMMatrixScaling(0.75, 0.75, 0.75);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureTF);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(35, 4, -20);
	S = XMMatrixScaling(1.2, 1.2, 1.2);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureMF);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(-30, -10, 0);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureE);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(-5, 0, -20);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferStar, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureSS);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(-30, -7, -5);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureBH);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(18,-40, -10);
	S = XMMatrixScaling(2.0, 2.0, 2.0);
	Mobject = Vc*S*T; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureA1);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	T = XMMatrixTranslation(4, 40, 0);
	S = XMMatrixScaling(1.0, 1.0, 1.0);
	//Rz = XMMatrixRotationZ(angle*10.0f);
	Mobject = Vc*S*T;// *Rz; //from left to right
	VsConstData.world = Mobject;

	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureS1);
	g_pImmediateContext->Draw(6, 0);		//render rectangle

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
	}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);	//message loop function (containing all switch-case statements

int WINAPI wWinMain(				//	the main function in a window program. program starts here
	HINSTANCE hInstance,			//	here the program gets its own number
	HINSTANCE hPrevInstance,		//	in case this program is called from within another program
	LPTSTR    lpCmdLine,
	int       nCmdShow)
	{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	hInst = hInstance;												//						save in global variable for further use
	MSG msg;

	// Globale Zeichenfolgen initialisieren
	LoadString(hInstance, 103, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, 104, szWindowClass, MAX_LOADSTRING);
	//register Window													<<<<<<<<<<			STEP ONE: REGISTER WINDOW						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	WNDCLASSEX wcex;												//						=> Filling out struct WNDCLASSEX
	BOOL Result = TRUE;
	wcex.cbSize = sizeof(WNDCLASSEX);								//						size of this struct (don't know why
	wcex.style = CS_HREDRAW | CS_VREDRAW;							//						?
	wcex.lpfnWndProc = (WNDPROC)WndProc;							//						The corresponding Proc File -> Message loop switch-case file
	wcex.cbClsExtra = 0;											//
	wcex.cbWndExtra = 0;											//
	wcex.hInstance = hInstance;										//						The number of the program
	wcex.hIcon = LoadIcon(hInstance, NULL);							//
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);						//
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);				//						Background color
	wcex.lpszMenuName = NULL;										//
	wcex.lpszClassName = L"TutorialWindowClass";									//						Name of the window (must the the same as later when opening the window)
	wcex.hIconSm = LoadIcon(wcex.hInstance, NULL);					//
	Result = (RegisterClassEx(&wcex) != 0);							//						Register this struct in the OS

																	//													STEP TWO: OPENING THE WINDOW with x,y position and xlen, ylen !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	RECT rc = { 0, 0, 1920, 1080 };//640,480 ... 1280,720
	hMain = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 2: Rendering a Triangle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (hMain == 0)	return 0;

	ShowWindow(hMain, nCmdShow);
	UpdateWindow(hMain);


	if (FAILED(InitDevice()))
		{
		return 0;
		}

	//													STEP THREE: Going into the infinity message loop							  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Main message loop
	msg = { 0 };
	while (WM_QUIT != msg.message)
		{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		else
			{
			Render();
			}
		}

	return (int)msg.wParam;
	}
///////////////////////////////////////////////////
void redr_win_full(HWND hwnd, bool erase)
	{
	RECT rt;
	GetClientRect(hwnd, &rt);
	InvalidateRect(hwnd, &rt, erase);
	}

///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
	{

	}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
	{
	if (x > 250)
		{
		PostQuitMessage(0);
		}

	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
	{


	}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////


void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
	{

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
		{
		}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
		{
		}
	}
///////////////////////////////////
//		This Function is called once at the begin of a program
///////////////////////////////////
#define TIMER1 1

BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
	{
	hMain = hwnd;
	return TRUE;
	}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
	HWND hwin;

	switch (id)
		{
			default:
				break;
		}

	}
//************************************************************************
void OnTimer(HWND hwnd, UINT id)
	{

	}
//************************************************************************
///////////////////////////////////
//		This Function is called every time the window has to be painted again
///////////////////////////////////


void OnPaint(HWND hwnd)
	{


	}
//****************************************************************************

//*************************************************************************
void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{

	switch (vk)
	{
	case 85:	//u
		cam.qk = 1;
		break;
	case 79:	//o
		cam.ek = 1;
		break;
	case 73:	//i
		cam.wk = 1;
		break;
	case 75:	//k
		cam.sk = 1;
		break;
	case 74:	//j
		cam.ak = 1;
		break;
	case 76:	//l
		cam.dk = 1;
		break;


	case 87:	//w
		plane_z_positive = true;
		break;
	case 83:	//s
		plane_z_negative = true;
		break;
	case 65:	//a
		plane_x_negative = true;
		break;
	case 68:	//d
		plane_x_positive = true;
		break;
	case 81:	//q
		plane_y_positive = true;
		break;
	case 69:	//e
		plane_y_negative = true;
		break;
		//case 90:	//z
	case 16://(UINT)(GetKeyState(VK_LSHIFT) & 0x8000) :	//shift
		boost = true;
		break;
	default:break;

	}
	}

//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
	switch (vk)
	{
	case 85:	//u
		cam.qk = 0;
		break;
	case 79:	//o
		cam.ek = 0;
		break;
	case 73:	//i
		cam.wk = 0;
		break;
	case 75:	//k
		cam.sk = 0;
		break;
	case 74:	//j
		cam.ak = 0;
		break;
	case 76:	//l
		cam.dk = 0;
		break;


	case 87:	//w
		plane_z_positive = false;
		break;
	case 83:	//s
		plane_z_negative = false;
		break;
	case 65:	//a
		plane_x_negative = false;
		break;
	case 68:	//d
		plane_x_positive = false;
		break;
	case 81://q
		plane_y_positive = false;
		break;
	case 69://e
		plane_y_negative = false;
		break;
		//case 90:	//z
	case 16:	//shift - ? DATA LINK ESCAPE
		boost = false;
		break;
	default:break;

	}

	}


//**************************************************************************
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{

	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
		{



		/*
		#define HANDLE_MSG(hwnd, message, fn)    \
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (fn))
		*/

		HANDLE_MSG(hwnd, WM_CHAR, OnChar);			// when a key is pressed and its a character
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLBD);	// when pressing the left button
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLBU);		// when releasing the left button
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMM);		// when moving the mouse inside your window
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);		// called only once when the window is created
													//HANDLE_MSG(hwnd, WM_PAINT, OnPaint);		// drawing stuff
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);	// not used
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKeyDown);	// press a keyboard key
		HANDLE_MSG(hwnd, WM_KEYUP, OnKeyUp);		// release a keyboard key
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);		// timer
			case WM_PAINT:
				hdc = BeginPaint(hMain, &ps);
				EndPaint(hMain, &ps);
				break;
			case WM_ERASEBKGND:
				return (LRESULT)1;
			case WM_DESTROY:
				PostQuitMessage(0);
				break;
			default:
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
	return 0;
	}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
		{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
		}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
	}