#include "KojinRenderer.h"
#include "SPIRVShader.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanRenderUnit.h"
#include "KojinCamera.h"
#include "Mesh.h"
#include "Material.h"
#include "SDL2\SDL_syswm.h"




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
		this->m_renderUnit->Initialize(m_system, m_commandUnit, m_imageUnit, m_swapChainUnit);
	}
	catch (...)
	{
		throw;
	}

	//add default camera 
	m_defaultCamera = std::make_shared<KojinCamera>(KojinCamera(this->m_swapChainUnit->swapChainExtent2D));
	m_defaultCamera->SetPerspective();
	//m_defaultCamera->SetOrthographic();
	m_defaultCamera->SetPosition({ 0,0,-1.0 });
	m_defaultCamera->LookAt({ 0, 0, 0});
	BindCamera(m_defaultCamera, true);

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
	if (meshDraws.count(meshID) == 0)
		meshDraws.insert(std::make_pair(meshID,1));
	else
		meshDraws[meshID]++;
	objectCount++;
	meshPartMaterials.push_back(material);
	meshPartTransforms.push_back(lockedMesh->modelMatrix);
}

void Vulkan::KojinRenderer::BindCamera(const std::weak_ptr<KojinCamera>& camera,bool isMainCamera)
{

	auto cam = camera.lock();
	if (!cam)
		return;

	if (isMainCamera)
	{
		this->m_renderUnit->SetAsMainCamera(cam->m_cameraID, 
			&cam->m_cameraViewport, &cam->m_cameraScissor,
			&cam->m_viewMatrix, &cam->m_projectionMatrix);
		return;
	}
	this->m_renderUnit->AddCamera(cam->m_cameraID, 
		&cam->m_cameraViewport, &cam->m_cameraScissor, 
		&cam->m_viewMatrix, &cam->m_projectionMatrix);
}

void Vulkan::KojinRenderer::UnbindCamera(std::weak_ptr<KojinCamera>& camera)
{

	auto cam = camera.lock();
	if (!cam)
		return;
	m_renderUnit->RemoveCamera(cam->m_cameraID);
}

void Vulkan::KojinRenderer::Render()
{

	m_renderUnit->ConsumeMesh(
		Mesh::m_iMeshVertices.data(),
		Mesh::m_iMeshVertices.size(),
		Mesh::m_iMeshIndices.data(),
		Mesh::m_iMeshIndices.size(),
		meshDraws,
		objectCount);
	m_renderUnit->SetTransformsAndMaterials(meshPartTransforms, meshPartMaterials);
	m_renderUnit->Render();
	meshDraws.clear();
	meshPartTransforms.clear();
	meshPartMaterials.clear();
	m_renderUnit->PresentFrame();

}

void Vulkan::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->m_system->GetLogicalDevice());
}

std::weak_ptr<Vulkan::KojinCamera> Vulkan::KojinRenderer::GetDefaultCamera()
{
	return m_defaultCamera;
}
