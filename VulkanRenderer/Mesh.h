#pragma once
#include "VulkanObject.h"
#include "VulkanSystemStructs.h"
#include <assimp\postprocess.h>
#include <vector>


struct aiScene;
namespace Vulkan
{

	class VulkanRenderUnit;
	class Mesh
	{

	public:

		///will consume scene pointer on load
		static Mesh* LoadMesh(const char * filename, int flags=defaultFlags);
		~Mesh();
		void BuildVertexBuffer();
		void BuildIndiceBuffer();
	private:
		Mesh();
		//hold references to all created meshes
		static std::vector<Mesh*> meshes;
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

		uint32_t vertCount;
		std::vector<VkVertex> vertices;
		std::vector<uint32_t> indices;
		static VulkanObjectContainer<VkDevice> * devicePtr;
		static VulkanRenderUnit * renderUnitPtr;

		VulkanObjectContainer<VkBuffer> vertexBuffer;
		VulkanObjectContainer<VkDeviceMemory> vertexBufferMemory;
		VulkanObjectContainer<VkBuffer> indiceBuffer;
		VulkanObjectContainer<VkDeviceMemory> indiceBufferMemory;
		uint32_t materialIndex;
		static void CleanUp();
		friend class VulkanRenderUnit;

	};
}