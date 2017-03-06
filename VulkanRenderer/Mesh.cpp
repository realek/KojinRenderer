#include"VKWorldSpace.h"
#include "Mesh.h"
#include "VulkanSystemStructs.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <unordered_map>
#include "VulkanHash.h"

std::atomic<int> Vulkan::Mesh::globalID = 0;
std::map<std::string,int> Vulkan::Mesh::m_loadedIMeshes;
std::map<int,Vulkan::IMeshData> Vulkan::Mesh::m_iMeshData;
std::vector<Vulkan::VkVertex> Vulkan::Mesh::m_iMeshVertices;
std::vector<uint32_t> Vulkan::Mesh::m_iMeshIndices;
#define PLANE_PATH "CODE_QUAD"
#define SPHERE_PATH "CODE_SPHERE"
#define CUBE_PATH "CODE_CUBE"

Vulkan::Mesh::Mesh() : m_meshID(++globalID)
{
}

Vulkan::Mesh::Mesh(int meshID)
{
	m_meshID = meshID;
}

int Vulkan::Mesh::GetID()
{
	return m_meshID;
}
Vulkan::IMeshData * Vulkan::Mesh::GetMeshData(int meshID)
{
	return &Mesh::m_iMeshData[meshID];
}
///asumes the file has one mesh within its scene, TODO:: add checks for loaded meshes, add checks for multiple meshes in one file(submesh concept)
std::shared_ptr<Vulkan::Mesh> Vulkan::Mesh::LoadMesh(const char * filename,int flags)
{
	if(m_loadedIMeshes.count(filename)!=0)
	{
		return std::make_shared<Vulkan::Mesh>(Mesh(m_loadedIMeshes[filename]));
	}
	else
	{
		std::unordered_map<Vulkan::VkVertex, uint32_t> uVertices = {}; // used to apply indexing
		auto iMesh = std::make_shared<Vulkan::Mesh>(Mesh());
		IMeshData meshData = {};
		meshData.vertexRange.start = m_iMeshVertices.size();
		meshData.indiceRange.start = m_iMeshIndices.size();
		std::vector<VkVertex> readVerts;
		std::vector<uint32_t> readIndices;

		{
			Assimp::Importer Importer;

			auto scene = Importer.ReadFile(filename, flags);

			if (!scene)
				throw std::runtime_error("Failed to read model file.");


			auto meshCount = scene->mNumMeshes;
			aiVector3D zeroVec(0.0f, 0.0f, 0.0f);
			for (uint32_t i = 0; i < meshCount; i++)
			{
				auto mesh = scene->mMeshes[i];
				readIndices.reserve(mesh->mNumVertices);
				readVerts.reserve(mesh->mNumVertices);
				for (uint32_t j = 0; j < mesh->mNumVertices; j++)
				{

					aiVector3D pos = mesh->mVertices[j];
					aiVector3D normal = mesh->mNormals[j];
					aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][j] : zeroVec;


					VkVertex vertex = {};

					vertex.pos = {
						pos.x,
						pos.y,
						pos.z
					};

					vertex.normal =
					{
						normal.x,
						normal.y,
						normal.z
					};

					vertex.texCoord = {
						texCoord.x,
						1.0 - texCoord.y
					};

					vertex.color = { 1.0f, 1.0f, 1.0f };

					if (uVertices.count(vertex) == 0)
					{
						uVertices[vertex] = readVerts.size();
						readVerts.push_back(vertex);
					}

					readIndices.push_back(uVertices[vertex]);
				}

				readVerts.shrink_to_fit();
				uVertices.clear();

				size_t currentSize = m_iMeshVertices.size();
				size_t neededSize = currentSize + readVerts.size();
				size_t currentCap = m_iMeshVertices.capacity();
				if (neededSize > currentCap)
					m_iMeshVertices.resize(neededSize);

				std::move(readVerts.begin(), readVerts.end(), m_iMeshVertices.begin() + currentSize);

				currentSize = m_iMeshIndices.size();
				neededSize = currentSize + readIndices.size();
				currentCap = m_iMeshIndices.capacity();
				if (neededSize > currentCap)
					m_iMeshIndices.resize(neededSize);

				std::move(readIndices.begin(), readIndices.end(), m_iMeshIndices.begin() + currentSize);
				meshData.indiceRange.end = m_iMeshIndices.size();
				meshData.vertexRange.end = m_iMeshVertices.size();
				meshData.vertexCount = readVerts.size();
				meshData.indiceCount = readIndices.size();
				m_iMeshData.insert(std::make_pair(iMesh->m_meshID, meshData));
				m_loadedIMeshes.insert(std::make_pair(filename, iMesh->m_meshID));

			}
		}
		return iMesh;
	}
}

std::shared_ptr<Vulkan::Mesh> Vulkan::Mesh::GetCube()
{

	if (m_loadedIMeshes.count(CUBE_PATH) != 0)
	{
		return std::make_shared<Vulkan::Mesh>(Mesh(m_loadedIMeshes[CUBE_PATH]));
	}
	else
	{
		auto iMesh = std::make_shared<Vulkan::Mesh>(Mesh());
		IMeshData meshData = {};
		meshData.vertexRange.start = m_iMeshVertices.size();
		meshData.indiceRange.start = m_iMeshIndices.size();
		float halfUnit = VkWorldSpace::UNIT/2;
		std::vector<VkVertex> verts
		{
			//back
			{ glm::vec3(halfUnit,halfUnit,-halfUnit), glm::vec3(0,0,-1), glm::vec3(1,1,1), glm::vec2(1,1) }, //v0
			{ glm::vec3(halfUnit,-halfUnit,-halfUnit), glm::vec3(0,0,-1), glm::vec3(1,1,1), glm::vec2(1,0) }, //v1
			{ glm::vec3(-halfUnit,-halfUnit,-halfUnit), glm::vec3(0,0,-1), glm::vec3(1,1,1), glm::vec2(0,0) }, //v2
			{ glm::vec3(-halfUnit,halfUnit,-halfUnit), glm::vec3(0,0,-1), glm::vec3(1,1,1), glm::vec2(0,1) }, //v3
			//left
			{ glm::vec3(halfUnit,halfUnit,halfUnit), glm::vec3(1,0,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v3
			{ glm::vec3(halfUnit,-halfUnit,halfUnit), glm::vec3(1,0,0), glm::vec3(1,1,1), glm::vec2(0,0) }, //v2
			{ glm::vec3(halfUnit,-halfUnit,-halfUnit), glm::vec3(1,0,0), glm::vec3(1,1,1), glm::vec2(0,0) }, //v4
			{ glm::vec3(halfUnit,halfUnit,-halfUnit), glm::vec3(1,0,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v6
			//top
			{ glm::vec3(-halfUnit,halfUnit,-halfUnit), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v6
			{ glm::vec3(-halfUnit,halfUnit,halfUnit), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v3
			{ glm::vec3(halfUnit,halfUnit,halfUnit), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(1,1) }, //v0
			{ glm::vec3(halfUnit,halfUnit,-halfUnit), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(1,1) }, //v7
			//front
			{ glm::vec3(halfUnit,halfUnit,halfUnit), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(1,1) }, //v7
			{ glm::vec3(-halfUnit,halfUnit,halfUnit), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(0,1) }, //v6
			{ glm::vec3(-halfUnit,-halfUnit,halfUnit), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(0,0) }, //v4
			{ glm::vec3(halfUnit,-halfUnit,halfUnit), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(1,0) }, //v5
			//right
			{ glm::vec3(-halfUnit,-halfUnit,halfUnit), glm::vec3(-1,0,0), glm::vec3(1,1,1), glm::vec2(1,0) }, //v5
			{ glm::vec3(-halfUnit,halfUnit,halfUnit), glm::vec3(-1,0,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v7
			{ glm::vec3(-halfUnit,halfUnit,-halfUnit), glm::vec3(-1,0,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v0
			{ glm::vec3(-halfUnit,-halfUnit,-halfUnit), glm::vec3(-1,0,0), glm::vec3(1,1,1), glm::vec2(1,0) }, //v1
			//bot
			{ glm::vec3(halfUnit,-halfUnit,halfUnit), glm::vec3(0,-1,0), glm::vec3(1,1,1), glm::vec2(1,0) }, //v1
			{ glm::vec3(-halfUnit,-halfUnit,halfUnit), glm::vec3(0,-1,0), glm::vec3(1,1,1), glm::vec2(0,0) }, //v2
			{ glm::vec3(-halfUnit,-halfUnit,-halfUnit), glm::vec3(0,-1,0), glm::vec3(1,1,1), glm::vec2(0,0) }, //v4
			{ glm::vec3(halfUnit,-halfUnit,-halfUnit), glm::vec3(0,-1,0), glm::vec3(1,1,1), glm::vec2(1,0) } //v5
		};

		std::vector<uint32_t> indices
		{ 
			0,1,2,0,2,3,
			4,5,6,4,6,7,
			8,9,10,8,10,11,
			12,13,14,12,14,15,
			16,17,18,16,18,19,
			20,21,22,20,22,23
		};

		size_t currentSize = m_iMeshVertices.size();
		size_t neededSize = currentSize + verts.size();
		size_t currentCap = m_iMeshVertices.capacity();
		if (neededSize > currentCap)
			m_iMeshVertices.resize(neededSize);

		std::move(verts.begin(), verts.end(), m_iMeshVertices.begin() + currentSize);

		currentSize = m_iMeshIndices.size();
		neededSize = currentSize + indices.size();
		currentCap = m_iMeshIndices.capacity();
		if (neededSize > currentCap)
			m_iMeshIndices.resize(neededSize);

		std::move(indices.begin(), indices.end(), m_iMeshIndices.begin() + currentSize);
		meshData.indiceRange.end = m_iMeshIndices.size();
		meshData.vertexRange.end = m_iMeshVertices.size();
		meshData.vertexCount = verts.size();
		meshData.indiceCount = indices.size();
		m_iMeshData.insert(std::make_pair(iMesh->m_meshID, meshData));
		m_loadedIMeshes.insert(std::make_pair(CUBE_PATH, iMesh->m_meshID));

		return iMesh;
	}
}

std::shared_ptr<Vulkan::Mesh> Vulkan::Mesh::GetSphere()
{
	if (m_loadedIMeshes.count(SPHERE_PATH) != 0)
	{
		return std::make_shared<Vulkan::Mesh>(Mesh(m_loadedIMeshes[SPHERE_PATH]));
	}
	else
	{
		auto iMesh = std::make_shared<Vulkan::Mesh>(Mesh());
		IMeshData meshData = {};
		meshData.vertexRange.start = m_iMeshVertices.size();
		meshData.indiceRange.end = m_iMeshIndices.size();
		std::vector<VkVertex> verts
		{
			{ glm::vec3(1,-1,0), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(1,0) },
			{ glm::vec3(1,1,0), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(1,1) },
			{ glm::vec3(-1,-1,0), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(0,0) },
			{ glm::vec3(-1,1,0), glm::vec3(0,0,1), glm::vec3(1,1,1), glm::vec2(0,1) }
		};

		std::vector<uint32_t> indices{ 0,1,2,2,1,3 };
		size_t currentSize = m_iMeshVertices.size();
		size_t neededSize = currentSize + verts.size();
		size_t currentCap = m_iMeshVertices.capacity();
		if (neededSize > currentCap)
			m_iMeshVertices.resize(neededSize);

		std::move(verts.begin(), verts.end(), m_iMeshVertices.begin() + currentSize);

		currentSize = m_iMeshIndices.size();
		neededSize = currentSize + indices.size();
		currentCap = m_iMeshIndices.capacity();
		if (neededSize > currentCap)
			m_iMeshIndices.resize(neededSize);

		std::move(indices.begin(), indices.end(), m_iMeshIndices.begin() + currentSize);
		meshData.indiceRange.end = m_iMeshIndices.size();
		meshData.vertexRange.end = m_iMeshVertices.size();
		meshData.vertexCount = verts.size();
		meshData.indiceCount = indices.size();
		m_iMeshData.insert(std::make_pair(iMesh->m_meshID, meshData));
		m_loadedIMeshes.insert(std::make_pair(SPHERE_PATH, iMesh->m_meshID));

		return iMesh;
	}
}

std::shared_ptr<Vulkan::Mesh> Vulkan::Mesh::GetPlane()
{
	if(m_loadedIMeshes.count(PLANE_PATH)!=0)
	{
		return std::make_shared<Vulkan::Mesh>(Mesh(m_loadedIMeshes[PLANE_PATH]));
	}
	else
	{
		auto iMesh = std::make_shared<Vulkan::Mesh>(Mesh());
		IMeshData meshData = {};
		meshData.vertexRange.start = m_iMeshVertices.size();
		meshData.indiceRange.start = m_iMeshIndices.size();
		std::vector<VkVertex> verts 
		{
			{ glm::vec3(VkWorldSpace::UNIT,0,VkWorldSpace::UNIT), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(1,1) }, //v0
			{ glm::vec3(-VkWorldSpace::UNIT,0,VkWorldSpace::UNIT), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(0,1) }, //v1
			{ glm::vec3(-VkWorldSpace::UNIT,0,-VkWorldSpace::UNIT), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(0,0) }, //v2
			{ glm::vec3(VkWorldSpace::UNIT,0,-VkWorldSpace::UNIT), glm::vec3(0,1,0), glm::vec3(1,1,1), glm::vec2(1,0) } //v3
		};
		std::vector<uint32_t> indices { 0,2,1,0,3,2 };
		size_t currentSize = m_iMeshVertices.size();
		size_t neededSize = currentSize + verts.size();
		size_t currentCap = m_iMeshVertices.capacity();
		if (neededSize > currentCap)
			m_iMeshVertices.resize(neededSize);

		std::move(verts.begin(), verts.end(), m_iMeshVertices.begin() + currentSize);

		currentSize = m_iMeshIndices.size();
		neededSize = currentSize + indices.size();
		currentCap = m_iMeshIndices.capacity();
		if (neededSize > currentCap)
			m_iMeshIndices.resize(neededSize);

		std::move(indices.begin(), indices.end(), m_iMeshIndices.begin() + currentSize);
		meshData.indiceRange.end = m_iMeshIndices.size();
		meshData.vertexRange.end = m_iMeshVertices.size();
		meshData.vertexCount = verts.size();
		meshData.indiceCount = indices.size();
		m_iMeshData.insert(std::make_pair(iMesh->m_meshID, meshData));
		m_loadedIMeshes.insert(std::make_pair(PLANE_PATH, iMesh->m_meshID));

		return iMesh;
	}
}

Vulkan::Mesh::~Mesh()
{

}