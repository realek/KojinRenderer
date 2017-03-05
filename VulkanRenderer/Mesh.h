/*=========================================================
Mesh.h - Abstraction class used to store model data such as
vertices, indices, normals from a file loaded with assimp.
==========================================================*/

#pragma once
#include <assimp\postprocess.h>
#include <vector>
#include <glm\matrix.hpp>
#include <memory>
#include <map>
#include <atomic>
struct aiScene;
namespace Vulkan
{
	struct IMeshData
	{
		glm::vec2 vertexRange;
		glm::vec2 indiceRange;
		uint32_t indiceCount;
		uint32_t vertexCount;
		uint32_t materialIndex; // to be used when creating materials via import
	};

	struct VkVertex;
	class Mesh
	{

	public:

		static std::shared_ptr<Vulkan::Mesh> LoadMesh(const char * filename, int flags=defaultFlags);
		//static std::shared_ptr<Vulkan::Mesh> GetCube();
		//static std::shared_ptr<Vulkan::Mesh> GetSphere();
		static std::shared_ptr<Vulkan::Mesh> GetPlane();
		~Mesh();
		glm::mat4 modelMatrix;
		int GetID();

		static IMeshData * GetMeshData(int meshID);
	private:
		Mesh();
		Mesh(int meshID);
		int m_meshID;
		static std::atomic<int> globalID;
		//default assimp import flags
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		static std::map<std::string, int> m_loadedIMeshes;
		static std::map<int,IMeshData> m_iMeshData;
		static std::vector<VkVertex> m_iMeshVertices;
		static std::vector<uint32_t> m_iMeshIndices;
		friend class KojinRenderer;

	};
}