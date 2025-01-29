#pragma once
#include <iostream>
#include "DX12/PCH.h"
#include "DX12/GraphicsTypes.h"
#include "assimp\Importer.hpp"      // C++ importer interface
#include "assimp\scene.h"           // Output data structure
#include "assimp\postprocess.h"     // Post processing flags


using namespace DirectX;

typedef UINT16 Index;
struct Vertex { float v1, v2, v3; };
/// Default struct for general vertex data include position, texture coordinates and normals
struct VertexType
{
	XMFLOAT3 position;
	//XMFLOAT2 texture;
	//XMFLOAT3 normal;
};

/// Default vertex struct for geometry with only position and colour
struct VertexType_Colour
{
	XMFLOAT3 position;
	XMFLOAT4 colour;
};

/// Default vertex struct for geometry with only position and texture coordinates.
struct VertexType_Texture
{
	XMFLOAT3 position;
	XMFLOAT2 texture;
};

enum MaterialTexture
{
    ALBEDO = 0,
    NORMAL,
    ROUGHNESS,
    METALLIC,
};
struct Mesh
{
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	uint32_t numVertices;
	uint32_t numIndices;
	uint32_t vertexOffset;
	uint32_t indexOffset;
};

class Model
{
public:
    Model(const std::string& file);
    ~Model();



	void ImportModel(const std::string& pFile);
	void ProcessNode(const aiNode* node, const aiScene* scene);
	void ProcessMesh(const aiMesh* mesh, const aiScene* scene);

	void InitFromAssimpMesh(const aiMesh& assimpMesh, Vertex* dstVertices, Index* dstIndices);
	void CreateBuffers();




	std::vector<Mesh> meshes;
	int meshCount = 0;

	ComPtr<ID3D12Resource> vertbuf;
	ComPtr<ID3D12Resource> indbuf;
	UINT vertsize;
	UINT indSize;
private:
	D3D12_VERTEX_BUFFER_VIEW vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indeexBuffer;

	//std::vector<Vertex> vertices;
	//std::vector<Index> indices;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBuffer;

	Vertex* vertices = nullptr;
	Index* indices = nullptr;

	GpuBuffer m_geometryBuffer;

	uint32_t vertexSizeInBytes;
	uint32_t indexSizeInBytes;


};