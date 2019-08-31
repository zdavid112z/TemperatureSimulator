#pragma once

#include "Pch.h"
#include <Windows.h>

#include <d3d11.h>
#include <DirectXMath.h>

class Mesh;
class Shader;
class Display;
class Texture2D;

class Graphics
{
public:
	Graphics(Display* display, uint adapterIndex);
	~Graphics();

	void BeginScene(DirectX::XMFLOAT3 clearColor);
	void Present();

	ID3D11Device* GetDevice() const { return m_Device; }
	ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext; }
	IDXGISwapChain* GetSwapChain() const { return m_SwapChain; }

	void Resize(uint width, uint height);
	void Render(Shader* shader, Mesh* mesh, Texture2D* texture);

	std::string GetVideoCardInfo();
	std::vector<std::pair<uint, std::string>> GetAdaptersInfo() { return m_AdaptersInfo; }
private:
	IDXGIAdapter* SelectAdapter(Display* display, uint adapterIndex);
private:
	Display* m_Display;

	std::pair<uint, uint> m_Refreshrate;
	bool m_VSync;
	IDXGISwapChain* m_SwapChain;
	ID3D11Device* m_Device;
	ID3D11DeviceContext* m_DeviceContext;
	ID3D11RenderTargetView* m_RenderTargetView;
	ID3D11Texture2D* m_DepthStencilBuffer;
	ID3D11DepthStencilState* m_DepthStencilState;
	ID3D11DepthStencilView* m_DepthStencilView;
	ID3D11RasterizerState* m_RasterState;
	std::string m_VideoCardInfo;
	uint m_VideoCardMemory;
	std::vector<std::pair<uint, std::string> > m_AdaptersInfo;

};