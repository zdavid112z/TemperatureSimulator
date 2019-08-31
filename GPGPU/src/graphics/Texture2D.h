#pragma once

#include "Pch.h"
#include <d3d11.h>

class Graphics;

class Texture2D
{
	friend class Graphics;
public:
	Texture2D(Graphics* g, uint width, uint height);
	~Texture2D();

	ID3D11Texture2D* GetTexture() { return m_Texture; }
	uint GetWidth() const { return m_Width; }
	uint GetHeight() const { return m_Height; }
private:
	uint m_Width, m_Height;
	ID3D11SamplerState* m_SamplerState;
	ID3D11Texture2D* m_Texture;
	ID3D11ShaderResourceView* m_TextureView;
};