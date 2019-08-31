#pragma once

#include "Pch.h"
#include "MeshFactory.h"
#include "Display.h"

class Mesh
{
	friend class Graphics;
public:
	Mesh(Graphics* g, const MeshData& data);
	~Mesh();

	void Bind(Graphics* g);
private:
	ID3D11Buffer *m_VertexBuffer, *m_IndexBuffer;
	uint m_VertexCount, m_IndexCount;
};