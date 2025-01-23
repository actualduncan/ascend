#pragma once
#include <iostream>
#include "DX12/PCH.h"
#include "assimp\Importer.hpp"      // C++ importer interface
#include "assimp\scene.h"           // Output data structure
#include "assimp\postprocess.h"     // Post processing flags
using namespace DirectX;


/// Default struct for general vertex data include position, texture coordinates and normals
struct VertexType
{
	XMFLOAT3 position;
	XMFLOAT2 texture;
	XMFLOAT3 normal;
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

class Model
{
public:
    Model(const std::string& file);
    ~Model();

	void ImportModel(const std::string& pFile);
	void ProcessNode(const aiNode* node, const aiScene* scene);
	void ProcessMesh(const aiMesh* mesh, const aiScene* scene);

	std::vector<VertexType> vertices;
	std::vector<unsigned long> indices;

private:
};