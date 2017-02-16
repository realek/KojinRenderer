#pragma once
#include <assimp\postprocess.h>
#include <vector>
#include <glm\matrix.hpp>
struct aiScene;
namespace Vulkan
{
	struct VkVertex;
	class Material;
	class Mesh
	{

	public:

		static Vulkan::Mesh* LoadMesh(const char * filename, int flags=defaultFlags);
		static Vulkan::Mesh* GetCube();
		static Vulkan::Mesh* GetSphere();
		static Vulkan::Mesh* GetPlane();
		~Mesh();
		Vulkan::Material * material;
		uint32_t vertCount;
		std::vector<VkVertex> vertices;
		std::vector<uint32_t> indices;

		//void BuildVertexBuffer();
		//void BuildIndiceBuffer();
		glm::mat4 modelMatrix;
	private:
		Mesh();
		uint32_t materialIndex;
		//hold references to all created meshes
		static std::vector<Mesh*> meshes;
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		static void CleanUp();

	};
}