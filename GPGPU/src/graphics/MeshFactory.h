#pragma once

#include "Pch.h"
#include <DirectXMath.h>

struct Vertex
{
	Vertex() {}
	Vertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& uv) :
		position(position), uv(uv) {}

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;
};

struct MeshData
{
	std::vector<Vertex> vertices;
	std::vector<uint32> indices;
};

class MeshFactory 
{
public:
	static MeshData GeneratePlane(float size);

};