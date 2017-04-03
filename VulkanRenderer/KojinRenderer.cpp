
#include "SPIRVShader.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanRenderUnit.h"
#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "Material.h"
#include "KojinRenderer.h"
#include "SDL2\SDL_syswm.h"
#include <algorithm>



Vulkan::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };


	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	this->m_system = std::make_shared<Vulkan::VulkanSystem>(window,
		appName,
		appVer,
		RENDER_ENGINE_NAME,
		engineVer);

	this->m_commandUnit = std::make_shared<Vulkan::VulkanCommandUnit>();
	this->m_imageUnit = std::make_shared<Vulkan::VulkanImageUnit>();
	this->m_swapChainUnit = std::make_shared<Vulkan::VulkanSwapchainUnit>();
	this->m_renderUnit = std::make_shared<Vulkan::VulkanRenderUnit>();

	try
	{
		this->m_system->Initialize(-1, nullptr, w, h);
		this->m_commandUnit->Initialize(m_system);
		this->m_imageUnit->Initialize(m_system, m_commandUnit);
		this->m_swapChainUnit->Initialize(m_system, m_commandUnit, m_imageUnit);
		this->m_renderUnit->Initialize(m_system, m_imageUnit, m_commandUnit, m_swapChainUnit);
	}
	catch (...)
	{
		throw;
	}

}

Vulkan::KojinRenderer::~KojinRenderer()
{

}

void Vulkan::KojinRenderer::Load(std::weak_ptr<Vulkan::Mesh> mesh, Vulkan::Material * material)
{
	auto lockedMesh = mesh.lock();

	if (!lockedMesh)
		throw std::runtime_error("Unable to lock weak ptr to provided mesh");

	if (m_meshDraws.count(lockedMesh->id) == 0)
		m_meshDraws.insert(std::make_pair(lockedMesh->id,1));
	else
		m_meshDraws[lockedMesh->id]++;
	m_objectCount++;
	m_meshPartMaterials.push_back(material);
	m_meshPartTransforms.push_back(lockedMesh->modelMatrix);
}

std::shared_ptr<Vulkan::Light> Vulkan::KojinRenderer::CreateLight(glm::vec3 initialPosition)
{
	auto light = std::make_shared<Vulkan::Light>(Light(m_renderUnit.get(),Vulkan::VulkanRenderUnit::RemoveLight,initialPosition));
	light->m_bound = true;
	m_renderUnit->AddLight(light.get());
	return light;
}

std::shared_ptr<Vulkan::Camera> Vulkan::KojinRenderer::CreateCamera(glm::vec3 initialPosition, bool perspective)
{
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(Camera(m_swapChainUnit->swapChainExtent2D, perspective, m_renderUnit.get(),VulkanRenderUnit::SetAsMainCamera, VulkanRenderUnit::RemoveCamera));
	camera->m_bound = true;
	camera->SetPositionRotation(initialPosition, { 0,0,0 });
	m_renderUnit->AddCamera(camera.get());

	return camera;
}

void Vulkan::KojinRenderer::Render()
{

	//m_renderUnit->SetLights(m_lights);
	m_renderUnit->ConsumeMesh(
		Mesh::m_iMeshVertices.data(),
		static_cast<uint32_t>(Mesh::m_iMeshVertices.size()),
		Mesh::m_iMeshIndices.data(),
		static_cast<uint32_t>(Mesh::m_iMeshIndices.size()),
		m_meshDraws,
		m_objectCount);
	m_renderUnit->SetTransformsAndMaterials(m_meshPartTransforms, m_meshPartMaterials);
	m_renderUnit->RecordCommandBuffers();
	m_meshDraws.clear();
	m_meshPartTransforms.clear();
	m_meshPartMaterials.clear();
	m_objectCount = 0;
	m_renderUnit->PresentFrame();

}

void Vulkan::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->m_system->GetLogicalDevice());
}
