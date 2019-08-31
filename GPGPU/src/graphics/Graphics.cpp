#include "Pch.h"
#include "Graphics.h"
#include "Display.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture2D.h"

Graphics::Graphics(Display* display, uint adapterIndex)
{
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_VIEWPORT viewport;

	IDXGIAdapter* adapter = SelectAdapter(display, adapterIndex);
	std::pair<uint, uint> refreshRate = m_Refreshrate;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = display->GetWidth();
	swapChainDesc.BufferDesc.Height = display->GetHeight();

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (display->m_VSync)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = refreshRate.first;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = refreshRate.second;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = display->m_Window;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	swapChainDesc.Windowed = !display->m_Fullscreen;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_SwapChain, &m_Device, NULL, &m_DeviceContext);
	assert(!FAILED(result));

	// Get the pointer to the back buffer.
	result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	assert(!FAILED(result));

	// Create the render target view with the back buffer pointer.
	result = m_Device->CreateRenderTargetView(backBufferPtr, NULL, &m_RenderTargetView);
	assert(!FAILED(result));

	backBufferPtr->Release();

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = display->GetWidth();
	depthBufferDesc.Height = display->GetHeight();
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = m_Device->CreateTexture2D(&depthBufferDesc, NULL, &m_DepthStencilBuffer);
	assert(!FAILED(result));

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilState);
	assert(!FAILED(result));

	m_DeviceContext->OMSetDepthStencilState(m_DepthStencilState, 1);
	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_Device->CreateDepthStencilView(m_DepthStencilBuffer, &depthStencilViewDesc, &m_DepthStencilView);
	assert(!FAILED(result));

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterState);
	assert(!FAILED(result));

	// Now set the rasterizer state.
	m_DeviceContext->RSSetState(m_RasterState);

	// Setup the viewport for rendering.
	viewport.Width = (float)display->GetWidth();
	viewport.Height = (float)display->GetHeight();
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_DeviceContext->RSSetViewports(1, &viewport);

	adapter->Release();
}

IDXGIAdapter* Graphics::SelectAdapter(Display* display, uint adapterIndex)
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	assert(!FAILED(result));

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(adapterIndex, &adapter);
	assert(!FAILED(result));

	IDXGIAdapter* currentAdapter;
	for (int index = 0; factory->EnumAdapters(index, &currentAdapter) != DXGI_ERROR_NOT_FOUND; index++)
	{
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0, returnedFeatureLevel;
		ID3D11Device* tempDevice;
		ID3D11DeviceContext* tempContext;
		result = D3D11CreateDevice(currentAdapter, D3D_DRIVER_TYPE_UNKNOWN, 
			NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, 
			&tempDevice, &returnedFeatureLevel, &tempContext);
		if (SUCCEEDED(result))
		{
			DXGI_ADAPTER_DESC desc;
			result = currentAdapter->GetDesc(&desc);
			assert(!FAILED(result));
			std::wstring winfo = desc.Description;
			std::string info(winfo.begin(), winfo.end());
			m_AdaptersInfo.push_back(std::make_pair(index, info));

			tempDevice->Release();
			tempContext->Release();
		}
		currentAdapter->Release();
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	assert(!FAILED(result));

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	assert(!FAILED(result));

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	assert(displayModeList);

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	assert(!FAILED(result));

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)display->GetWidth())
		{
			if (displayModeList[i].Height == (unsigned int)display->GetHeight())
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	assert(!FAILED(result));

	// Store the dedicated video card memory in megabytes.
	int videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);
	char videoCardDescription[128];
	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, videoCardDescription, 128, adapterDesc.Description, 128);
	assert(!error);

	m_VideoCardMemory = videoCardMemory;
	m_VideoCardInfo = std::string(videoCardDescription);

	// Release the display mode list.
	delete[] displayModeList;
	adapterOutput->Release();
	factory->Release();

	m_Refreshrate = std::make_pair(numerator, denominator);
	return adapter;
}

Graphics::~Graphics()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	m_SwapChain->SetFullscreenState(false, NULL);
	m_RasterState->Release();
	m_DepthStencilView->Release();
	m_DepthStencilState->Release();
	m_DepthStencilBuffer->Release();
	m_RenderTargetView->Release();
	m_DeviceContext->Release();
	m_Device->Release();
	m_SwapChain->Release();
}

void Graphics::BeginScene(DirectX::XMFLOAT3 clearColor)
{
	float color[4];

	// Setup the color to clear the buffer to.
	color[0] = clearColor.x;
	color[1] = clearColor.y;
	color[2] = clearColor.z;
	color[3] = 1;

	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

	// Clear the back buffer.
	m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, color);

	// Clear the depth buffer.
	m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Graphics::Present()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_VSync)
	{
		// Lock to screen refresh rate.
		m_SwapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_SwapChain->Present(0, 0);
	}
}

void Graphics::Resize(uint width, uint height)
{
	D3D11_VIEWPORT viewport;
	HRESULT result;
	m_DeviceContext->OMSetRenderTargets(0, 0, 0);
	if (m_RenderTargetView)
	{
		m_RenderTargetView->Release();
		m_RenderTargetView = NULL;
	}
	result = m_SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	assert(!FAILED(result));

	ID3D11Texture2D* pBackBuffer;
	result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(void**)&pBackBuffer);
	assert(!FAILED(result));

	result = m_Device->CreateRenderTargetView(pBackBuffer, NULL, &m_RenderTargetView);
	assert(!FAILED(result));

	pBackBuffer->Release();

	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, NULL);

	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_DeviceContext->RSSetViewports(1, &viewport);
}

std::string Graphics::GetVideoCardInfo()
{
	return m_VideoCardInfo + "\nDedicated memory: " + std::to_string(m_VideoCardMemory) + "MB\n";
}

void Graphics::Render(Shader* shader, Mesh* mesh, Texture2D* texture)
{
	mesh->Bind(this);
	shader->Bind(this);

	m_DeviceContext->PSSetSamplers(0, 1, &texture->m_SamplerState);
	m_DeviceContext->PSSetShaderResources(0, 1, &texture->m_TextureView);

	m_DeviceContext->DrawIndexed(mesh->m_IndexCount, 0, 0);
}