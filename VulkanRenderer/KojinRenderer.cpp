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



void Vulkan::KojinStagingObject::UpdateUniforms(KojinStagingObject& updated)
{
	this->modelMatrices = updated.modelMatrices;
	this->diffuseColors = updated.diffuseColors;
	this->specularities = updated.specularities;
	this->diffuseTextures = updated.diffuseTextures;
}

void Vulkan::KojinStagingObject::ClearTemporary()
{
	vertex.clear();
	indices.clear();
}

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
	m_defaultCamera->LookAt({ 0, 0.25, 0 });
	BindCamera(m_defaultCamera, true);

	//init staging object
	m_stagingOld = {};
	m_stagingCurrent = {};

}

Vulkan::KojinRenderer::~KojinRenderer()
{
}

void Vulkan::KojinRenderer::Load(std::weak_ptr<Vulkan::Mesh> mesh, std::weak_ptr<Vulkan::Material> material)
{
	auto lockedMesh = mesh.lock();
	auto lockedMat = material.lock();

	if (!lockedMesh)
		throw std::runtime_error("Unable to lock weak ptr to provided mesh");
	if (!lockedMat)
		throw std::runtime_error("Unable to lock weak ptr to provided material");
	
	m_stagingCurrent.ids.push_back(lockedMesh->GetID());
	m_stagingCurrent.vertex.insert(m_stagingCurrent.vertex.end(),lockedMesh->vertices.begin(),lockedMesh->vertices.end());
	m_stagingCurrent.indices.insert(m_stagingCurrent.indices.end(), lockedMesh->indices.begin(), lockedMesh->indices.end());
	m_stagingCurrent.indiceOffset.push_back(m_stagingCurrent.totalIndices);
	m_stagingCurrent.totalIndices += lockedMesh->indices.size();
	m_stagingCurrent.modelMatrices.push_back(lockedMesh->modelMatrix);
	m_stagingCurrent.diffuseColors.push_back(lockedMat->diffuseColor);
	m_stagingCurrent.specularities.push_back(lockedMat->specularity);
	m_stagingCurrent.diffuseTextures.push_back(lockedMat->diffuseTexture);
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

void Vulkan::KojinRenderer::DrawSingleObject(uint64_t texture, Vulkan::Mesh * mesh)
{
	m_renderUnit->Render(texture, mesh);
}

void Vulkan::KojinRenderer::Update(float deltaTime)
{
	m_renderUnit->UpdateStaticUniformBuffer(deltaTime);
}

void Vulkan::KojinRenderer::Render()
{
	bool recreateBuffers = false;
	if (m_stagingCurrent != m_stagingOld)
	{
		m_stagingOld.indiceOffset = m_stagingCurrent.indiceOffset;
		m_stagingOld.ids = m_stagingCurrent.ids;
		m_stagingOld.totalIndices = m_stagingCurrent.totalIndices;
		recreateBuffers = true;
	}

	m_renderUnit->ConsumeMesh(recreateBuffers);
	m_stagingCurrent = {}; // clear current staging object

	//m_renderUnit->Render(nullptr, nullptr);
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
