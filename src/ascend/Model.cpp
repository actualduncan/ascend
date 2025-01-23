#include "Model.h"
#include "DX12/DX12.h"


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
		ProcessNode(scene->mRootNode, scene);
	}

	// create vertex buffer and what not
	
}


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
		XMFLOAT3 vert(0.0f, 0.0f, 0.0f);
		XMFLOAT2 text(0.0f,0.0f);
		XMFLOAT3 norm(0.0f, 0.0f, 0.0f);

		vert.x = mesh->mVertices[i].x;
		vert.y = mesh->mVertices[i].y;
		vert.z = mesh->mVertices[i].z;

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

		VertexType vertex;
		vertex.position = vert;
		vertex.texture = text;
		vertex.normal = norm;
		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
}