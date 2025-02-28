#include "Model.h"
#include "DX12/DX12.h"
#include "DX12/DX12_Helpers.h"
#include "DX12/Textures.h"

D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexBufferView()
{
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = vertexBufferAddress;
	vbView.SizeInBytes = numVertices * sizeof(Vertex);
	vbView.StrideInBytes = sizeof(Vertex);
	return vbView;
}

D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView()
{
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	ibView.BufferLocation = indexBufferAddress;
	ibView.SizeInBytes = numIndices * sizeof(Index);
	ibView.Format = DXGI_FORMAT_R16_UINT;
	return ibView;
}

Model::Model(const std::string& file)
{
	ImportModel(file);
}

Model::~Model()
{

}

void Model::ImportModel(const std::string& file, const std::string& textureDirectory)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(file,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs);


	if (scene)
	{
		// Initialize the meshes
		const uint64_t numMeshes = scene->mNumMeshes;
	
		uint64_t numVertices = 0;
		uint64_t numIndices = 0;
		for (uint64_t i = 0; i < numMeshes; ++i)
		{
			const aiMesh& assimpMesh = *scene->mMeshes[i];

			numVertices += assimpMesh.mNumVertices;
			numIndices += assimpMesh.mNumFaces * 3;

		}

		vertices = new Vertex[numVertices * sizeof(Vertex)];
		indices = new Index[numIndices * sizeof(Index)];

		uint64_t vtxOffset = 0;
		uint64_t idxOffset = 0;

		for (uint64_t i = 0; i < numMeshes; ++i)
		{
			InitFromAssimpMesh(*scene->mMeshes[i], &vertices[vtxOffset], &indices[idxOffset]);

			vtxOffset += m_modelMeshes[i].numVertices;
			idxOffset += m_modelMeshes[i].numIndices;
		}

		CreateBuffers();	
	}

	const uint64_t numMaterials = scene->mNumMaterials;
	m_modelMaterials.Init(numMaterials);

	for(uint64_t i = 0; i < numMaterials; ++i)
	{
		Material& material = m_modelMaterials[i];
		const aiMaterial& mat = *scene->mMaterials[i];

// read model material file names and find associated textures to link
		aiString matName;
		mat.Get(AI_MATKEY_NAME, matName);
		material.Name = matName.C_Str();

		aiString diffuseTexturePath;
		aiString normalMapPath;
		if(mat.GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath) == aiReturn_SUCCESS)
		{
			// std::wstring diffusePath;
			// diffusePath = LPCTSTR(diffuseTexturePath.C_Str());
			material.TextureNames[uint64_t(MaterialTextures::ALBEDO)] = LPCTSTR(diffuseTexturePath.C_Str());
		}

		if(mat.GetTexture(aiTextureType_NORMALS, 0, &normalMapPath) == aiReturn_SUCCESS)
		{
			// std::wstring normalPath;
			// normalPath = LPCTSTR(normalMapPath.C_Str());
			material.TextureNames[uint64_t(MaterialTextures::NORMAL)] = LPCTSTR(normalMapPath.C_Str());
		}
		
	}
	LoadTextures(textureDirectory);
}

void Model::LoadTextures(const std::string& fileDirectory)
{
	// LoadTextures probably some issues with c_str and wstring stuff 
	// have fun future duncan :)
	const uint64_t numMaterials = m_modelMaterials.Size();

	for(uint64_t matIdx = 0; matIdx < numMaterials; ++matIdx)
	{
		Material& material = m_modelMaterials[matIdx];
		for(uint64_t texType = 0 ; texType < MaterialTextures::COUNT; ++texType)
		{
			material.Textures[texType] = nullptr;

			std::wstring path = LPCTSTR(fileDirectory.c_str()) + material.TextureNames[texType];
			if(material.TextureNames[texType].length == 0)
			{
				path = L"debug/res/textures/empty.dds";
			}

			// Check if this texture has already been loaded.
			const uint64_t numLoaded = m_materialTextures.size();
			for(uint64_t i = 0; i < numLoaded; ++i)
			{
				if(materialtextures[i]->Name == path)
				{
					material.Textures[texType] = &materialTextures[i]->Texture;
					break;
				}
			}

			if(material.Textures[texType] == nullptr)
			{
				MaterialTexture* newMatTexture = new MaterialTexture();
				newMatTexture->Name = path;
				LoadTextureFromFile(newMatTexture->Texture, path.c_str());
				material.Textures[texType] = &newMatTexture;
				uint64_t idx = m_materialTextures.push_back(newMatTexture);
			}
		}
	}
	/*
	Some notes on what happens 
	iterate through each material ->
	iterate through each materialTexture ->
	set wstring variable ot keep track of texture name + path
	if file doesn't exist/isn't set make default texture
	
	check if materialtexture has been intialised
	- Create new texture
	- Set name ot the path of hte texture
	Load texture function 
	- selects to load by DDS extension creates texture as a placed resource and outputs to referenced texture
	- uploades texture (similar ot GraphicsTypes.h Texture class constructor <- use this but with a default constructor ideally.)
	set id to material textures.add return (vector iterator after push_back)
	set texture for as a reference ot this new texture created
	set Texture index to types idx

	
	*/
}

void Model::CreateBuffers()
{
	uint64_t vertexBufferSize = 0;
	uint64_t indexBufferSize = 0;

	for (auto it = m_modelMeshes.begin(); it < m_modelMeshes.end(); ++it)
	{

		it->vertexOffset = vertexBufferSize;
		it->indexOffset = indexBufferSize;

		vertexBufferSize += it->numVertices * sizeof(Vertex);
		indexBufferSize += it->numIndices * sizeof(Index);
	}

	AllocateUploadBuffer(DX12::Device.Get(), vertices, vertexBufferSize, &m_vertexBuffer);
	AllocateUploadBuffer(DX12::Device.Get(), indices, indexBufferSize, &m_indexBuffer);

	for (auto it = m_modelMeshes.begin(); it < m_modelMeshes.end(); ++it)
	{
		it->indexBufferAddress = m_indexBuffer->GetGPUVirtualAddress() + it->indexOffset;
		it->vertexBufferAddress = m_vertexBuffer->GetGPUVirtualAddress() + it->vertexOffset;
	}
}

void Model::InitFromAssimpMesh(const aiMesh& assimpMesh, Vertex* dstVertices, Index* dstIndices)
{
	Mesh mesh;

	mesh.numVertices = assimpMesh.mNumVertices;
	mesh.numIndices = assimpMesh.mNumFaces * 3;

	for (uint64_t i = 0; i < mesh.numVertices; ++i)
	{
		Vertex vert;

		vert.Position.x = assimpMesh.mVertices[i].x * 0.3;
		vert.Position.y = assimpMesh.mVertices[i].y * 0.3;
		vert.Position.z = assimpMesh.mVertices[i].z * 0.3;

		vert.Normal.x = assimpMesh.mNormals[i].x;
		vert.Normal.y = assimpMesh.mNormals[i].y;
		vert.Normal.z = assimpMesh.mNormals[i].z;

		vert.Tangent.x = assimpMesh.mTangents[i].x;
		vert.Tangent.y = assimpMesh.mTangents[i].y;
		vert.Tangent.z = assimpMesh.mTangents[i].z;

		vert.BitTangent.x = assimpMesh.mBitangents[i].x;
		vert.BitTangent.y = assimpMesh.mBitangents[i].y;
		vert.BitTangent.z = assimpMesh.mBitangents[i].z;
		
		vert.TexC.x = assimpMesh.mTextureCoords[0][i].x;
		vert.TexC.y = assimpMesh.mTextureCoords[0][i].y;

		dstVertices[i] = vert;
	}

	const uint64_t numTriangles = assimpMesh.mNumFaces;
	for (uint64_t triIdx = 0; triIdx < numTriangles; ++triIdx)
	{
		dstIndices[triIdx * 3 + 0] = UINT16(assimpMesh.mFaces[triIdx].mIndices[0]);
		dstIndices[triIdx * 3 + 1] = UINT16(assimpMesh.mFaces[triIdx].mIndices[1]);
		dstIndices[triIdx * 3 + 2] = UINT16(assimpMesh.mFaces[triIdx].mIndices[2]);
	}

	mesh.materialIdx = assimpMesh.mMaterialIndex;

	m_modelMeshes.push_back(mesh);
}

D3D12_VERTEX_BUFFER_VIEW Model::GetVertexBuffer()
{
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = m_vertexBuffer->GetDesc().Width;
	vbView.StrideInBytes = sizeof(Vertex);
	return vbView;
}

D3D12_INDEX_BUFFER_VIEW Model::GetIndexBuffer()
{
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	ibView.SizeInBytes = m_indexBuffer->GetDesc().Width;
	ibView.Format = DXGI_FORMAT_R16_UINT;
	return ibView;
}