#pragma once
#include <iostream>
#include "DX12/PCH.h"
#include "DX12/GraphicsTypes.h"
#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags

using namespace DirectX;

typedef UINT16 Index;
struct Vertex { 
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
};

struct Mesh
{
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();

	uint32_t numVertices;
	uint32_t numIndices;
	uint32_t vertexOffset;
	uint32_t indexOffset;

	D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAddress;
	D3D12_GPU_VIRTUAL_ADDRESS indexBufferAddress;
};

class Model
{
public:
    Model(const std::string& file);
    ~Model();

	std::vector<Mesh>& GetModelMeshVector() { return m_modelMeshes; };
	D3D12_VERTEX_BUFFER_VIEW GetVertexBuffer();
	D3D12_INDEX_BUFFER_VIEW GetIndexBuffer();
private:
	void ImportModel(const std::string& pFile);
	void InitFromAssimpMesh(const aiMesh& assimpMesh, Vertex* dstVertices, Index* dstIndices);
	void CreateBuffers();

	std::vector<Mesh> m_modelMeshes;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;

	Vertex* vertices = nullptr;
	Index* indices = nullptr;

};