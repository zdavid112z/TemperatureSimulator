#pragma once

#include "Pch.h"
#include "Display.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include "Camera.h"

struct ShaderBuffer
{
	DirectX::XMMATRIX modelMatrix;
	DirectX::XMMATRIX viewProjectionMatrix;
};

class Shader
{
	friend class Graphics;
public:
	Shader(Graphics* g, const std::wstring& vertShaderPath, const std::wstring& pixelShaderPath);
	~Shader();

	void UpdateBuffer(Graphics* g, Camera* camera, const DirectX::XMMATRIX& modelMatrix = DirectX::XMMatrixIdentity());
	void Bind(Graphics* g);
private:
	void OutputShaderError(ID3D10Blob* errorMessage, const std::wstring& shaderPath);
private:
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;
	ID3D11InputLayout* m_InputLayout;
	ID3D11Buffer* m_InputBuffer;
};