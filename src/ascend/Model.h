#pragma once
#include <iostream>
#include "DX12/PCH.h"
#include "DX12/GraphicsTypes.h"
#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags

using namespace DirectX;

enum class MaterialTextures
{
	ALBEDO = 0,
	NORMAL,

	COUNT
};

struct MaterialTexture
{
    std::string Name;
    Texture Texture;
};

struct Material
{
	std::string Name;
	std::string TextureNames[uint64_t(MaterialTextures::COUNT)];
	const MaterialTexture* Textures[uint64_t(MaterialTextures::COUNT)] = {};

	// Used to grab the specific texture type needed. 
	const MaterialTexture* Texture(MaterialTextures textureType)
	{
		return Textures[uint64_t(textureType)];
	}

};

typedef UINT16 Index;
struct Vertex { 
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT3 Tangent;
	XMFLOAT3 BitTangent;
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

	int materialIdx;
};

class Model
{
public:
    Model(const std::string& file, const std::string& textureDirectory);
    ~Model();

	std::vector<Mesh>& GetModelMeshVector() { return m_modelMeshes; };
	D3D12_VERTEX_BUFFER_VIEW GetVertexBuffer();
	D3D12_INDEX_BUFFER_VIEW GetIndexBuffer();
		std::vector<Material> m_modelMaterials;
private:
	void ImportModel(const std::string& file, const std::string& textureDirectory);
	void LoadTextures(const std::string& fileDirectory);
	void InitFromAssimpMesh(const aiMesh& assimpMesh, Vertex* dstVertices, Index* dstIndices);
	void CreateBuffers();

	std::vector<Mesh> m_modelMeshes;
	
	std::vector<MaterialTexture*> m_materialTextures; 
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;

	Vertex* vertices = nullptr;
	Index* indices = nullptr;

};