#pragma once
#include <assimp\postprocess.h>
#include <vector>
#include <glm\matrix.hpp>
#include <memory>
#include <atomic>
struct aiScene;
namespace Vulkan
{
	struct VkVertex;
	class Mesh
	{

	public:

		static std::shared_ptr<Vulkan::Mesh> LoadMesh(const char * filename, int flags=defaultFlags);
		static Vulkan::Mesh* GetCube();
		static Vulkan::Mesh* GetSphere();
		static Vulkan::Mesh* GetPlane();
		~Mesh();
		uint32_t vertCount;
		std::vector<VkVertex> vertices;
		std::vector<uint32_t> indices;
		glm::mat4 modelMatrix;
		int GetID();
	private:
		Mesh();
		int m_meshID;
		uint32_t m_consumedPosition;
		uint32_t materialIndex;
		static std::atomic<int> globalID;
		//default assimp import flags
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	};
}