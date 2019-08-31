#include "Pch.h"
#include "Texture2D.h"
#include "Graphics.h"

Texture2D::Texture2D(Graphics* g, uint width, uint height) :
	m_Width(width), m_Height(height)
{
	HRESULT result;
	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	g->GetDevice()->CreateSamplerState(&samplerDesc, &m_SamplerState);

	// Setup the description of the texture.
	textureDesc.Height = height;
	textureDesc.Width = width;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	// Create the empty texture.
	result = g->GetDevice()->CreateTexture2D(&textureDesc, NULL, &m_Texture);
	assert(!FAILED(result));

	// Setup the shader resource view description.
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	result = g->GetDevice()->CreateShaderResourceView(m_Texture, &srvDesc, &m_TextureView);
}

Texture2D::~Texture2D()
{
	m_SamplerState->Release();
	m_Texture->Release();
	m_TextureView->Release();
}
