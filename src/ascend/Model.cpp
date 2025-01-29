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
	Assimp::Importer importer;

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

			vtxOffset += m_modelMeshes[i].numVertices;
			idxOffset += m_modelMeshes[i].numIndices;
		}

		CreateBuffers();	
	}
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