#include "Mesh.h"
#include "VulkanSystemStructs.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <unordered_map>
#include <VulkanHash.h>
std::atomic<int> Vulkan::Mesh::globalID = 0;


Vulkan::Mesh::Mesh() : vertCount(0), m_meshID(++globalID)
{
}

int Vulkan::Mesh::GetID()
{
	return m_meshID;
}
///asumes the file has one mesh within its scene
std::shared_ptr<Vulkan::Mesh> Vulkan::Mesh::LoadMesh(const char * filename,int flags)
{

	std::unordered_map<Vulkan::VkVertex, uint32_t> uVertices = {}; // used to apply indexing
	auto iMesh = std::make_shared<Vulkan::Mesh>(Mesh());

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
			iMesh->vertCount += mesh->mNumVertices;
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
					uVertices[vertex] = iMesh->vertices.size();
					iMesh->vertices.push_back(vertex);
				}

				iMesh->indices.push_back(uVertices[vertex]);
			}

			uVertices.clear();
		}
	}

	

	//iMesh->vertexBuffer = VulkanObjectContainer<VkBuffer>{ devicePtr,vkDestroyBuffer };
	//iMesh->vertexBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ devicePtr,vkFreeMemory };
	//iMesh->indiceBuffer = VulkanObjectContainer<VkBuffer>{ devicePtr,vkDestroyBuffer };
	//iMesh->indiceBufferMemory = VulkanObjectContainer<VkDeviceMemory>{ devicePtr,vkFreeMemory };

	//try
	//{
	//	iMesh->BuildVertexBuffer();
	//	iMesh->BuildIndiceBuffer();
	//}
	//
	//(std::runtime_error e)
	//{
	//	throw e;
	//}
	return iMesh;

}

Vulkan::Mesh * Vulkan::Mesh::GetCube()
{
	return nullptr;
}

Vulkan::Mesh * Vulkan::Mesh::GetSphere()
{
	return nullptr;
}

Vulkan::Mesh * Vulkan::Mesh::GetPlane()
{
	return nullptr;
}

Vulkan::Mesh::~Mesh()
{

}

/*
inline void Vulkan::Mesh::BuildVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	//VulkanObjectContainer<VkBuffer> stagingBuffer{ devicePtr, vkDestroyBuffer };
	//VulkanObjectContainer<VkDeviceMemory> stagingBufferMemory{ devicePtr, vkFreeMemory };

	//renderUnitPtr->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	//auto device = devicePtr->Get();
	void* data;
	
	//vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
//	vkUnmapMemory(device, stagingBufferMemory);

//	renderUnitPtr->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

//	renderUnitPtr->CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}
inline void Vulkan::Mesh::BuildIndiceBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

//	VulkanObjectContainer<VkBuffer> stagingBuffer{ devicePtr, vkDestroyBuffer };
//	VulkanObjectContainer<VkDeviceMemory> stagingBufferMemory{ devicePtr, vkFreeMemory };
//	renderUnitPtr->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

//	auto device = devicePtr->Get();
	void* data;
//	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
//	vkUnmapMemory(device, stagingBufferMemory);
//	renderUnitPtr->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indiceBuffer, indiceBufferMemory);

//	renderUnitPtr->CopyBuffer(stagingBuffer, indiceBuffer, bufferSize);
}
*/