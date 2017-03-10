#include "KojinRenderer.h"
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
		this->m_swapChainUnit->Initialize(m_system, m_imageUnit);
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

	auto meshID = lockedMesh->GetID();
	if (m_meshDraws.count(meshID) == 0)
		m_meshDraws.insert(std::make_pair(meshID,1));
	else
		m_meshDraws[meshID]++;
	m_objectCount++;
	m_meshPartMaterials.push_back(material);
	m_meshPartTransforms.push_back(lockedMesh->modelMatrix);
}

std::shared_ptr<Vulkan::Light> Vulkan::KojinRenderer::CreateLight(glm::vec3 initialPosition)
{
	auto light = std::make_shared<Vulkan::Light>(Light(this,KojinRenderer::ClearLight,initialPosition));
	light->m_bound = true;
	m_lights.push_back(light.get());
	return light;
}

void Vulkan::KojinRenderer::ClearLight(Vulkan::Light * light, Vulkan::KojinRenderer* rend)
{
	rend->m_lights.erase(std::remove(rend->m_lights.begin(), rend->m_lights.end(), light), rend->m_lights.end());
}

std::shared_ptr<Vulkan::KojinCamera> Vulkan::KojinRenderer::CreateCamera(glm::vec3 initialPosition, bool perspective)
{
	auto camera = std::make_shared<KojinCamera>(KojinCamera(this,KojinRenderer::BindCamera,KojinRenderer::UnbindCamera,this->m_swapChainUnit->swapChainExtent2D,perspective));
	camera->SetPosition(initialPosition);
	camera->BindSelf();
	return camera;
}

void Vulkan::KojinRenderer::BindCamera(KojinRenderer* rend, KojinCamera* cam)
{
	rend->m_renderUnit->AddCamera(cam->m_cameraID, 
		&cam->m_cameraViewport, &cam->m_cameraScissor, 
		&cam->m_viewMatrix, &cam->m_projectionMatrix,&cam->m_position);
}

void Vulkan::KojinRenderer::UnbindCamera(KojinRenderer* rend, KojinCamera* cam)
{
	rend->m_renderUnit->RemoveCamera(cam->m_cameraID);
}

void Vulkan::KojinRenderer::Render()
{

	m_renderUnit->ConsumeMesh(
		Mesh::m_iMeshVertices.data(),
		Mesh::m_iMeshVertices.size(),
		Mesh::m_iMeshIndices.data(),
		Mesh::m_iMeshIndices.size(),
		m_meshDraws,
		m_objectCount);
	m_renderUnit->SetTransformsAndMaterials(m_meshPartTransforms, m_meshPartMaterials);
	m_renderUnit->SetLights(m_lights);
	m_renderUnit->Render();
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

std::weak_ptr<Vulkan::KojinCamera> Vulkan::KojinRenderer::GetMainCamera()
{
	return m_mainCamera;
}

void Vulkan::KojinRenderer::SetMainCamera(std::shared_ptr<KojinCamera> camera)
{
	m_mainCamera = camera;
}


