#include "Pch.h"
#include "MeshFactory.h"

using namespace DirectX;

MeshData MeshFactory::GeneratePlane(float size)
{
	MeshData data;
	data.vertices.push_back(
		Vertex(XMFLOAT3(-size, -size, 0), XMFLOAT2(0, 0)));
	data.vertices.push_back(
		Vertex(XMFLOAT3( size, -size, 0), XMFLOAT2(1, 0)));
	data.vertices.push_back(
		Vertex(XMFLOAT3( size,  size, 0), XMFLOAT2(1, 1)));
	data.vertices.push_back(
		Vertex(XMFLOAT3(-size,  size, 0), XMFLOAT2(0, 1)));

	data.indices = { 0, 1, 2, 0, 2, 3 };
	return data;
}