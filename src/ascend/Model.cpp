#include "Model.h"
#include "DX12/DX12.h"
#include "DX12/DX12_Helpers.h"

Model::Model(const std::string& file)
{
	ImportModel(file);
}

Model::~Model()
{

}

void Model::ImportModel(const std::string& pFile)
{
	// Create an instance of the Importer class
	Assimp::Importer importer;
	// And have it read the given file with some example postprocessing
	// Usually - if speed is not the most important aspect for you - you'll
	// probably to request more postprocessing than we do in this example.
	const aiScene* scene = importer.ReadFile(pFile,
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
			meshCount++;

			vtxOffset += meshes[i].numVertices;
			idxOffset += meshes[i].numIndices;
		}

		CreateBuffers();	
	}

	
}

void Model::CreateBuffers()
{
	uint64_t vertexBufferSize = 0;
	uint64_t indexBufferSize = 0;

	for (auto it = meshes.begin(); it < meshes.end(); ++it)
	{
		it->vertexOffset = vertexBufferSize;
		it->indexOffset = indexBufferSize;

		vertexBufferSize += it->numVertices * sizeof(Vertex);
		indexBufferSize += it->numIndices * sizeof(Index);
	}

	AllocateUploadBuffer(DX12::Device.Get(), vertices, vertexBufferSize, &vertbuf);
	AllocateUploadBuffer(DX12::Device.Get(), indices, indexBufferSize, &indbuf);

}
void Model::InitFromAssimpMesh(const aiMesh& assimpMesh, Vertex* dstVertices, Index* dstIndices)
{
	Mesh mesh;

	mesh.numVertices = assimpMesh.mNumVertices;
	mesh.numIndices = assimpMesh.mNumFaces * 3;

	for (uint64_t i = 0; i < mesh.numVertices; ++i)
	{
		Vertex vert(0.0f, 0.0f, 0.0f);

		vert.v1 = assimpMesh.mVertices[i].x;
		vert.v2 = assimpMesh.mVertices[i].y;
		vert.v3 = assimpMesh.mVertices[i].z;

		dstVertices[i] = vert;
	}
	const uint64_t numTriangles = assimpMesh.mNumFaces;
	for (uint64_t triIdx = 0; triIdx < numTriangles; ++triIdx)
	{
		dstIndices[triIdx * 3 + 0] = UINT16(assimpMesh.mFaces[triIdx].mIndices[0]);
		dstIndices[triIdx * 3 + 1] = UINT16(assimpMesh.mFaces[triIdx].mIndices[1]);
		dstIndices[triIdx * 3 + 2] = UINT16(assimpMesh.mFaces[triIdx].mIndices[2]);
	}

	meshes.push_back(mesh);
}

/**
void Model::ProcessNode(const aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(mesh, scene);
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		this->ProcessNode(node->mChildren[i], scene);
	}
}

void Model::ProcessMesh(const aiMesh* mesh, const aiScene* scene)
{
	

	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vert(0.0f, 0.0f, 0.0f);
		XMFLOAT2 text(0.0f,0.0f);
		XMFLOAT3 norm(0.0f, 0.0f, 0.0f);

		vert.v1 = mesh->mVertices[i].x;
		vert.v2 = mesh->mVertices[i].y;
		vert.v3 = mesh->mVertices[i].z;

		if (mesh->HasTextureCoords(0))
		{
			text.x = (float)mesh->mTextureCoords[0][i].x;
			text.y = (float)mesh->mTextureCoords[0][i].y;
		}

		if (mesh->HasNormals())
		{
			norm.x = mesh->mNormals[i].x;
			norm.y = mesh->mNormals[i].y;
			norm.z = mesh->mNormals[i].z;
		}

		Vertex vertex;
		vertex = vert;
		//vertex.texture = text;
		//vertex.normal = norm;

	}


	
	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
		{
			newMesh.Indices[i * 3 + j] = UINT16(face.mIndices[j]);
			indSize++;
		}
	}

}
*/