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
	struct UIntRange
	{
		uint32_t start;
		uint32_t end;
	};
	struct IMeshData
	{
		UIntRange vertexRange;
		UIntRange indiceRange;
		uint32_t indiceCount;
		uint32_t vertexCount;
		uint32_t materialIndex; // to be used when creating materials via import
	};
	class Material;
	struct VkVertex;
	class Mesh
	{

	public:

		static std::shared_ptr<Vulkan::Mesh> LoadMesh(const char * filename, int flags=defaultFlags);
		static std::shared_ptr<Vulkan::Mesh> GetCube();
		static std::shared_ptr<Vulkan::Mesh> GetSphere();
		
		static std::shared_ptr<Vulkan::Mesh> GetPlane();
		~Mesh();
		glm::mat4 modelMatrix;
		const uint32_t id;
		static IMeshData * GetMeshData(uint32_t meshID);
	private:
		Mesh();
		Mesh(uint32_t meshID);
		static void WriteToInternalMesh(const char* filepath, std::vector<Vulkan::VkVertex>& verts, std::vector<uint32_t>& indices, std::shared_ptr<Vulkan::Mesh>& mesh);
	
	private:
		Material * m_material = nullptr;
		static std::atomic<uint32_t> globalID;
		//default assimp import flags
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		static std::map<std::string, uint32_t> m_loadedIMeshes;
		static std::map<uint32_t,IMeshData> m_iMeshData;
		static std::vector<VkVertex> m_iMeshVertices;
		static std::vector<uint32_t> m_iMeshIndices;
		static const std::string PLANE_PATH;
		static const std::string CUBE_PATH;
		static const std::string SPHERE_PATH;
		friend class KojinRenderer;

	};
}