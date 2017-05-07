#include "KojinRenderer.h"
#include "VkManagedInstance.h"
#include "VkManagedValidationLayers.h"
#include "VkManagedDevice.h"
#include "VkManagedQueue.h"
#include "VkManagedCommandPool.h"
#include "VkManagedCommandBuffer.h"
#include "VkManagedSwapchain.h"
#include "VkManagedDescriptorPool.h"
#include "VkManagedDescriptorSet.h"
#include "VkManagedSemaphore.h"
#include "VkManagedSampler.h"
#include "VkManagedRenderPass.h"
#include "VkManagedPipeline.h"

#include "SPIRVShader.h"
#include "Camera.h"
#include "Light.h"
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"

#include <SDL2\SDL_syswm.h>
#include <algorithm>
#include <functional>
#include <SDL2\SDL.h>
#include <SDL_image.h>

using namespace std::placeholders;

Vulkan::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };
	int w, h;
	SDL_GetWindowSize(window, &w, &h);


	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = appName;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(appVer[0], appVer[1], appVer[2]);
	applicationInfo.pEngineName = RENDER_ENGINE_NAME;
	applicationInfo.engineVersion = VK_MAKE_VERSION(engineVer[0], engineVer[1], engineVer[2]);
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	try
	{
		m_vkInstance = new VkManagedInstance(&applicationInfo, { "VK_LAYER_LUNARG_standard_validation" });
		m_vkInstance->MakeSurface(window, w, h);
		m_vkDevice = m_vkInstance->CreateVkManagedDevice(0, { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, true, true, false, false, false);
		m_vkPresentQueue = m_vkDevice->GetQueue(VK_QUEUE_GRAPHICS_BIT, true, false, true);
		m_vkMainCmdPool = new VkManagedCommandPool(m_vkDevice, m_vkDevice->GetQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, false, false, true));
		m_vkSwapchain = new VkManagedSwapchain(m_vkDevice, m_vkMainCmdPool, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_UNDEFINED);
		m_vkRenderpassFWD = new VkManagedRenderPass(m_vkDevice);
		m_vkRenderpassFWD->Build(m_vkSwapchain->Extent(), VK_FORMAT_B8G8R8A8_UNORM, m_vkDevice->Depthformat());
		m_vkRenderpassFWD->SetFrameBufferCount(1, true, false, true, false, false);
		m_vkPipelineFWD = new VkManagedPipeline(m_vkDevice);
		
		VkPushConstantRange rangeView;
		rangeView.offset = 0;
		rangeView.size = 64;
		rangeView.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		VkPushConstantRange rangeProj;
		rangeProj.offset = 64;
		rangeProj.size = 128;
		rangeProj.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		m_vkPipelineFWD->Build(
			m_vkRenderpassFWD, PipelineMode::Solid,
			"shaders/vertex.vert.spv",
			"shaders/fragment.frag.spv",
			{ VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_VIEWPORT
			}, {rangeView,rangeProj});

		//m_vkRenderPassSDWProj = new VkManagedRenderPass(m_vkDevice);
		//m_vkRenderPassSDWProj->Build({ VkShadowmapDefaults::k_resolution,VkShadowmapDefaults::k_resolution }, VkShadowmapDefaults::k_attachmentDepthFormat);
		//m_vkPipelineSDWProj = new VkManagedPipeline(m_vkDevice);
		//m_vkPipelineSDWProj->Build(m_vkRenderPassSDWProj, PipelineMode::ProjectedShadows,
		//	"shaders/vertexSkeleton.vert.spv",
		//	"shaders/fragmentSkeleton.frag.spv",
		//	{ VK_DYNAMIC_STATE_SCISSOR,
		//	VK_DYNAMIC_STATE_VIEWPORT,
		//	VK_DYNAMIC_STATE_DEPTH_BIAS
		//	});

		m_vkDescriptorPool = new VkManagedDescriptorPool(m_vkDevice);
		m_vkDescriptorPool->SetDescriptorCount(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);
		m_vkDescriptorPool->SetDescriptorCount(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);
		m_semaphores = new VkManagedSemaphore(m_vkDevice, 2); // 1 present and 2 pass 
		m_vkMainCmdPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkSwapchain->ImageCount(), m_swapChainbuffers);
		m_colorSampler = new VkManagedSampler(m_vkDevice, VkManagedSamplerMode::COLOR_NORMALIZED_COORDINATES, 16, VkBorderColor::VK_BORDER_COLOR_INT_OPAQUE_BLACK);
	}
	catch(...)
	{

		throw;
	}
	

}

Vulkan::KojinRenderer::~KojinRenderer()
{
	Clean();
	delete(m_semaphores);
	delete(m_meshIndexData);
	delete(m_meshVertexData);
	delete(m_vkRenderpassFWD);
	delete(m_vkRenderPassSDWProj);
	delete(m_vkPipelineFWD);
	delete(m_vkPipelineSDWProj);
	delete(m_vkDescriptorPool);
	delete(m_vkSwapchain);
	delete(m_vkMainCmdPool);
	delete(m_vkDevice);
	delete(m_vkInstance);
}

void Vulkan::KojinRenderer::Draw(std::vector<Mesh*> meshes, std::vector<Material*> materials)
{
	size_t meshSize = meshes.size();
	assert(meshSize == materials.size());
	
	m_meshPartTransforms.reserve(meshSize);
	m_meshPartMaterials.reserve(meshSize);

	for(Mesh* mesh : meshes)
	{
		if (std::find_if(m_meshDraws.begin(),m_meshDraws.end(),
			[&mesh](std::pair<const uint32_t, int> a)->bool { return a.first == mesh->id; })!= m_meshDraws.end())
		{
			m_meshDraws[mesh->id]++;
		}
		else
		{
			m_meshDraws.insert(std::make_pair(mesh->id, 1));
		}
		m_objectCount++;
		m_meshPartTransforms.push_back(mesh->modelMatrix);
	}

	for(Material*mat : materials)
		m_meshPartMaterials.push_back(mat);;
	
}
//void Vulkan::KojinRenderer::Load(std::weak_ptr<Vulkan::Mesh> mesh, Vulkan::Material * material)
//{
//	auto lockedMesh = mesh.lock();
//
//	if (!lockedMesh)
//		throw std::runtime_error("Unable to lock weak ptr to provided mesh");
//
//	if (m_meshDraws.count(lockedMesh->id) == 0)
//		m_meshDraws.insert(std::make_pair(lockedMesh->id,1));
//	else
//		m_meshDraws[lockedMesh->id]++;
//	m_objectCount++;
//	m_meshPartMaterials.push_back(material);
//	m_meshPartTransforms.push_back(lockedMesh->modelMatrix);
//}

Vulkan::Camera * Vulkan::KojinRenderer::CreateCamera(glm::vec3 initialPosition, bool perspective)
{
	Camera * c = new Camera(m_vkSwapchain->Extent(), perspective, std::bind(&KojinRenderer::FreeCamera, this, _1));
	c->m_position = initialPosition;
	c->m_rotation = { 0,0,0 };
	m_cameras.insert(std::make_pair(c->id, c));
	return c;
}

void Vulkan::KojinRenderer::FreeCamera(Camera * camera)
{
	m_cameras.erase(camera->id);
}

Vulkan::Light * Vulkan::KojinRenderer::CreateLight(glm::vec3 initialPosition)
{
	Light * l = new Light(initialPosition, std::bind(&KojinRenderer::FreeLight, this, _1));
	m_lights.insert(std::make_pair(l->id, l));
	return l;
}

void Vulkan::KojinRenderer::Clean()
{
	//if resources werent deallocated clean them
	{
		std::vector<void*> clears;
		if (m_cameras.size() > 0)
		{
			for (auto& cam : m_cameras)
			{
				clears.push_back(cam.second);
			}
		}

		if (m_lights.size() > 0)
		{
			for (auto& l : m_lights)
			{
				clears.push_back(l.second);
			}
		}
		if (clears.size() > 0)
		{
			for(void* clear : clears)
			{
				delete(clear);
			}
		}
	}


	if(m_virtualTextures.size() > 0)
	{
		for(auto& vTex : m_virtualTextures)
		{
			delete(vTex.second);
		}
		m_deviceLoadedTextures.clear();
	}
}

void Vulkan::KojinRenderer::FreeLight(Light * light)
{
	//auto res = std::find_if(m_lights.begin(), m_lights.end(), [&light](std::pair<const uint32_t, Vulkan::Light *> a)->bool { return a.first == light->id; });
	m_lights.erase(light->id);
}

Vulkan::Texture * Vulkan::KojinRenderer::LoadTexture(std::string filepath, bool readWrite)
{
	if (filepath.empty())
	{
		throw std::invalid_argument("Empty filepath provided. Texture load aborted.");
	}
	void * pixels = nullptr;
	auto surf = IMG_Load(filepath.c_str());

	if (surf == nullptr)
	{
		throw std::runtime_error("Unable to load texture image.");
	}

	uint32_t bytesPerPixel = static_cast<uint32_t>(surf->format->BytesPerPixel);
	if (readWrite)
	{
		uint32_t size = static_cast<uint32_t>(surf->w * surf->h * bytesPerPixel);
		pixels = new char[size] {0};
		memcpy(pixels, surf->pixels, size);
	}
	
	VkFormat colorFormat;
	switch (bytesPerPixel)
	{
	case 4:
		colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case 8:
		colorFormat = VK_FORMAT_R16G16B16A16_UNORM;
		break;
	case 16:
		colorFormat = VK_FORMAT_R32G32B32A32_UINT;
		break;
	default:
		throw std::runtime_error("Unsupported color format, unable to load file: "+ filepath);
		break;
	}
	
	Vulkan::Texture * texture = new Vulkan::Texture(pixels,static_cast<uint32_t>(surf->w),static_cast<uint32_t>(surf->h), bytesPerPixel);
	auto image = std::make_shared<VkManagedImage>(m_vkDevice);
	m_virtualTextures.insert(std::make_pair(texture->id, texture));
	m_deviceLoadedTextures.insert(std::make_pair(texture->id,image));
	image->Build({ static_cast<uint32_t>(surf->w),static_cast<uint32_t>(surf->h) }, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkManagedCommandBuffer cmdbuff = m_vkMainCmdPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkManagedQueue * cmdPoolQueue = m_vkMainCmdPool->PoolQueue();
	image->LoadData(&cmdbuff,0,cmdPoolQueue, surf->pixels, bytesPerPixel, static_cast<uint32_t>(surf->w), static_cast<uint32_t>(surf->h), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	cmdbuff.Free();
	SDL_free(surf);
	return texture;
}
Vulkan::Texture * Vulkan::KojinRenderer::GetTextureWhite()
{
	if (m_whiteTexture != nullptr)
		return m_whiteTexture;

	uint32_t w, h;
	w = h = 512;
	std::vector<unsigned char> pixels(w * h * 4, (char)0xff);
	m_whiteTexture = new Vulkan::Texture(nullptr, w, h, 4);
	auto image = std::make_shared<VkManagedImage>(m_vkDevice);
	m_deviceLoadedTextures.insert(std::make_pair(m_whiteTexture->id, image));
	image->Build({w,h }, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkManagedCommandBuffer cmdbuff = m_vkMainCmdPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkManagedQueue * cmdPoolQueue = m_vkMainCmdPool->PoolQueue();
	image->LoadData(&cmdbuff, 0, cmdPoolQueue, pixels.data(), 4, w, h, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	cmdbuff.Free();

	return m_whiteTexture;
}
void Vulkan::KojinRenderer::FreeTexture(Texture * tex) 
{
	assert(tex != nullptr);
	assert(tex->id != m_whiteTexture->id); // can't delete internal texture
	assert(m_deviceLoadedTextures.count(tex->id) != 0);
	m_deviceLoadedTextures.erase(tex->id);
	delete tex;
}

void Vulkan::KojinRenderer::Render()
{
	//acquire image
	uint32_t scImage = 0;
	m_vkSwapchain->AcquireNextImage(&scImage, m_semaphores->Next()); //handle sc return

	bool rebuild = false;
	if(m_objectCount != m_objectCountOld)
	{
		m_objectCountOld = m_objectCount;
		this->UpdateInternalMesh(m_vkMainCmdPool, 
			Mesh::m_iMeshVertices.data(), static_cast<uint32_t>(Mesh::m_iMeshVertices.size()), 
			Mesh::m_iMeshIndices.data(), static_cast<uint32_t>(Mesh::m_iMeshIndices.size()));
		
		if(m_vkDescriptorPool->Size() < m_objectCount*3)
		{
			m_vkDescriptorPool->BuildPool(m_objectCount*3);
			m_vkDescriptorPool->AllocateDescriptorSet(m_objectCount, m_vkPipelineFWD->GetVertexLayout(), m_vDescriptorSetFWD);
			m_vkDescriptorPool->AllocateDescriptorSet(m_objectCount, m_vkPipelineFWD->GetFragmentLayout(),m_fDescriptorSetFWD);
			//m_vkDescriptorPool->AllocateDescriptorSet(m_objectCount, m_vkPipelineSDWProj->GetVertexLayout(), m_vDescriptorSetFWD);
			CreateUniformBufferSet(m_uniformVStagingBufferFWD,m_uniformVBuffersFWD,m_objectCount,sizeof(VertexShaderMVP));
			CreateUniformBufferSet(m_uniformFStagingBufferFWD, m_uniformFBuffersFWD, m_objectCount, sizeof(LightingUniformBuffer));
			//CreateUniformBufferSet(m_uniformStagingBufferSDWProj, m_uniformBuffersSDWProj, m_objectCount, sizeof(VertexDepthMVP));
			//make sure to delete all buffers on clean function call
		
		}

		rebuild = true;
	}
	UpdateShadowmapLayers();

	//update uniform buffers

	//write descriptors here!
	for (uint32_t oc = 0; oc < m_objectCountOld; ++oc)
	{
		WriteDescriptors(oc);
	}

	std::vector<VkClearValue> clearValues;
	clearValues.resize(2);
	clearValues[0].color = { 0,0,0,1.0 };
	clearValues[1].depthStencil = { (uint32_t)1.0f, (uint32_t)0.0f };

	std::vector<VkIndexedDraw> indexdraws;
	indexdraws.resize(m_objectCount);
	{
		uint32_t objIndex = 0;
		for(std::pair<const uint32_t,int>& pair : m_meshDraws)
		{
			IMeshData * meshD = Mesh::GetMeshData(pair.first);
			for(int i = 0; i < pair.second;++i)
			{
				indexdraws[objIndex].indexCount = meshD->indiceCount;
				indexdraws[objIndex].indexStart = meshD->indiceRange.start;
				indexdraws[objIndex].vertexOffset = meshD->vertexRange.start;
				objIndex++;
			}



		}
	}
	//set states for the forward render pipeline
	VkDynamicStatesBlock states;
	states.viewports.resize(1);
	states.scissors.resize(1);
	states.hasViewport = VK_TRUE;
	states.hasScissor = VK_TRUE;

	m_swapChainbuffers->Begin(VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	for(uint32_t cmdIndex = 0; cmdIndex < m_swapChainbuffers->Size(); ++cmdIndex)
	{
		VkCommandBuffer cBuffer = m_swapChainbuffers->Buffer(cmdIndex);
		for (std::pair<uint32_t, Camera*> camera : m_cameras)
		{
			for (uint32_t oc = 0; oc < m_objectCountOld; ++oc)
			{
				UpdateUniformBuffer(cBuffer, oc, m_meshPartTransforms[oc], camera.second->m_viewMatrix, camera.second->m_projectionMatrix, m_meshPartMaterials[oc]);
			}

			states.viewports[0] = camera.second->m_viewPort;
			states.scissors[0] = camera.second->m_scissor;

			m_vkRenderpassFWD->SetPipeline(m_vkPipelineFWD, states, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS);
			m_vkRenderpassFWD->PreRecordData(cBuffer, 0); //we have 1 framebuffer
			std::vector<VkPushConstant> constants(2);
			constants[0].data = &camera.second->m_viewMatrix;
			constants[0].offset = 0;
			constants[0].size = sizeof(camera.second->m_viewMatrix);
			constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			constants[1].data = &camera.second->m_projectionMatrix;
			constants[1].offset = constants[0].size;
			constants[1].size = sizeof(camera.second->m_projectionMatrix);
			constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			m_vkRenderpassFWD->Record(clearValues, { m_vDescriptorSetFWD,m_fDescriptorSetFWD }, constants, m_meshIndexData, m_meshVertexData, indexdraws);

			//copy pass result
			VkManagedImage* passColor = m_vkRenderpassFWD->GetAttachment(0, VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			VkManagedImage* scImage = m_vkSwapchain->SwapchainImage(cmdIndex);
			passColor->Copy(cBuffer, scImage);
		}
	}

	m_swapChainbuffers->End();

	//submit & wait

	m_swapChainbuffers->Submit(m_vkPresentQueue->queue, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_semaphores->Next() }, { m_semaphores->Last() });
	//present
	m_vkSwapchain->PresentCurrentImage(&scImage, m_vkPresentQueue, { m_semaphores->Last()}); // pass waiting semaphores

	m_objectCount = 0;
	m_meshDraws.clear();
	m_meshPartMaterials.clear();
	m_meshPartTransforms.clear();
}

void Vulkan::KojinRenderer::WaitForIdle()
{
	m_vkDevice->WaitForIdle();
}

void Vulkan::KojinRenderer::UpdateInternalMesh(VkManagedCommandPool * commandPool, VkVertex * vertexData, uint32_t vertexCount, uint32_t * indiceData, uint32_t indiceCount)
{

	VkDeviceSize vertexSize = sizeof(vertexData[0]) * vertexCount;
	VkDeviceSize indiceSize = sizeof(indiceData[0]) * indiceCount;

	if (m_meshVertexData == nullptr && m_meshIndexData == nullptr)
	{
		m_meshIndexData = new VkManagedBuffer{ m_vkDevice };
		m_meshVertexData = new VkManagedBuffer{ m_vkDevice };
	}
	//create vertex and index buffers for the mesh
	//declare staging buffers
	VkManagedBuffer vertexStagingBuffer(m_vkDevice);
	VkManagedBuffer indiceStagingBuffer(m_vkDevice);

	//create staging buffers
	vertexStagingBuffer.Build(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexSize);
	indiceStagingBuffer.Build(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indiceSize);

	//copy data into staging buffers
	vertexStagingBuffer.Write(0, 0, (size_t)vertexStagingBuffer.bufferSize, vertexData);
	indiceStagingBuffer.Write(0, 0, (size_t)indiceStagingBuffer.bufferSize, indiceData);

	//create and load normal buffers
	m_meshVertexData->Build(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexSize);
	m_meshIndexData->Build(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indiceSize);

	VkManagedCommandBuffer cmdBuff = commandPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	cmdBuff.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
	VkCommandBuffer buffer = cmdBuff.Buffer();
	vertexStagingBuffer.CopyTo(buffer, m_meshVertexData, 0, 0, vertexSize);
	indiceStagingBuffer.CopyTo(buffer, m_meshIndexData, 0, 0, indiceSize);
	cmdBuff.End(0);
	cmdBuff.Submit(commandPool->PoolQueue()->queue, {}, {}, {}, 0);
	m_vkDevice->WaitForIdle();
	cmdBuff.Free();
}

void Vulkan::KojinRenderer::UpdateUniformBuffer(VkCommandBuffer recordBuffer, uint32_t bufferIndex, const glm::mat4 & model, const glm::mat4 & view, const glm::mat4 & proj, const Vulkan::Material * material)
{
	Vulkan::VertexShaderMVP ubo = {};
	ubo.model = model;
	ubo.ComputeMVP(view, proj);
	size_t dataSize = sizeof(ubo);
	m_uniformVStagingBufferFWD->Write(0, 0, dataSize, &ubo);

	try
	{
		m_uniformVStagingBufferFWD->CopyTo(recordBuffer, m_uniformVBuffersFWD[bufferIndex], 0, 0,dataSize);
	}
	catch (...)
	{
		throw;
	}

	Vulkan::LightingUniformBuffer lightsUbo = {};
	lightsUbo.ambientLightColor = glm::vec4(0.1, 0.1, 0.1, 0.1);
	lightsUbo.materialDiffuse = material->diffuseColor;
	lightsUbo.specularity = material->specularity;
	uint32_t i = 0;
	for (std::pair<const uint32_t,Light*>& l : m_lights)
	{
		if (i < MAX_LIGHTS_PER_FRAGMENT)
		{
			lightsUbo.lights[i] = {};
			lightsUbo.lights[i].color = l.second->diffuseColor;
			lightsUbo.lights[i].direction = view*l.second->GetLightForward();
			lightsUbo.lights[i].m_position = glm::vec4(l.second->m_position, 1.0f);
			lightsUbo.lights[i].m_position.x *= -1;
			lightsUbo.lights[i].m_position = view*lightsUbo.lights[i].m_position;
			lightsUbo.lights[i].lightProps = {};
			lightsUbo.lights[i].lightProps.lightType = l.second->GetType();
			lightsUbo.lights[i].lightProps.intensity = l.second->intensity;
			lightsUbo.lights[i].lightProps.falloff = l.second->range;
			lightsUbo.lights[i].lightProps.angle = l.second->angle;
		//	if (i < depthMVPs.size())
		//		lightsUbo.lights[i].lightBiasedMVP = VkShadowmapDefaults::k_shadowBiasMatrix * (depthMVPs[i] * modelMatrix);
		//	else
				lightsUbo.lights[i].lightBiasedMVP = glm::mat4(1);
			i++;
		}
		else
			break;
	}
	dataSize = sizeof(LightingUniformBuffer);
	m_uniformFStagingBufferFWD->Write(0, 0, dataSize, &lightsUbo);

	try
	{
		m_uniformFStagingBufferFWD->CopyTo(recordBuffer, m_uniformFBuffersFWD[bufferIndex], 0, 0,dataSize);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::KojinRenderer::WriteDescriptors(uint32_t objIndex)
{
	//VERTEX
	VkDescriptorSet set = m_vDescriptorSetFWD->Set(objIndex);
	VkDescriptorBufferInfo vertexUniformBufferInfo = {};
	vertexUniformBufferInfo.buffer = *m_uniformVBuffersFWD[objIndex];
	vertexUniformBufferInfo.offset = 0;
	vertexUniformBufferInfo.range = sizeof(VertexShaderMVP);

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = set;
		descriptorVertexWrite.dstBinding = 0;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pBufferInfo = &vertexUniformBufferInfo;
	}

	vkUpdateDescriptorSets(*m_vkDevice, 1, &descriptorVertexWrite, 0, nullptr);

	//FRAGMENT
	set = m_fDescriptorSetFWD->Set(objIndex);
	VkDescriptorImageInfo diffuseTextureInfo = {};
	diffuseTextureInfo.imageLayout = m_deviceLoadedTextures[m_meshPartMaterials[objIndex]->albedo->id]->layout;
	diffuseTextureInfo.imageView = *m_deviceLoadedTextures[m_meshPartMaterials[objIndex]->albedo->id];
	diffuseTextureInfo.sampler = *m_colorSampler;

//	VkDescriptorImageInfo shadowMaptexture = {};
//	shadowMaptexture.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	//shadowMaptexture.imageView = m_fwdOffScreenProjShadows.GetAttachment(0, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)->imageView;
//	shadowMaptexture.imageView = m_layeredProjectedShadowmaps->imageView;
//	shadowMaptexture.sampler = m_fwdOffScreenProjShadows.GetSampler(m_fwdOffScreenProjShadows.k_defaultSamplerName);

	VkDescriptorBufferInfo lightsUniformBufferInfo = {};
	lightsUniformBufferInfo.buffer = *m_uniformFBuffersFWD[objIndex];
	lightsUniformBufferInfo.offset = 0;
	lightsUniformBufferInfo.range = sizeof(LightingUniformBuffer);


	std::array<VkWriteDescriptorSet, 2> fragmentDescriptorWrites = {};

	{
		fragmentDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[0].dstSet = set;
		fragmentDescriptorWrites[0].dstBinding = 0;
		fragmentDescriptorWrites[0].dstArrayElement = 0;
		fragmentDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fragmentDescriptorWrites[0].descriptorCount = 1;
		fragmentDescriptorWrites[0].pImageInfo = &diffuseTextureInfo;

		//fragmentDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//fragmentDescriptorWrites[1].dstSet = fragSet;
		//fragmentDescriptorWrites[1].dstBinding = 1;
		//fragmentDescriptorWrites[1].dstArrayElement = 0;
		//fragmentDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		//fragmentDescriptorWrites[1].descriptorCount = 1;
		//fragmentDescriptorWrites[1].pImageInfo = &shadowMaptexture;

	//	fragmentDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	fragmentDescriptorWrites[1].dstSet = set;
	//	fragmentDescriptorWrites[1].dstBinding = 1;
	//	fragmentDescriptorWrites[1].dstArrayElement = 0;
	//	fragmentDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//	fragmentDescriptorWrites[1].descriptorCount = 1;
	//	fragmentDescriptorWrites[1].pImageInfo = &diffuseTextureInfo;

		fragmentDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[1].dstSet = set;
		fragmentDescriptorWrites[1].dstBinding = 1;
		fragmentDescriptorWrites[1].dstArrayElement = 0;
		fragmentDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		fragmentDescriptorWrites[1].descriptorCount = 1;
		fragmentDescriptorWrites[1].pBufferInfo = &lightsUniformBufferInfo;
	}

	vkUpdateDescriptorSets(*m_vkDevice, static_cast<uint32_t>(fragmentDescriptorWrites.size()), fragmentDescriptorWrites.data(), 0, nullptr);
}

bool Vulkan::KojinRenderer::UpdateShadowmapLayers()
{
	if(m_lights.size() != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Vulkan::KojinRenderer::CreateUniformBufferSet(VkManagedBuffer *& stagingBuffer, std::vector<VkManagedBuffer*>& buffers, uint32_t objectCount, uint32_t bufferDataSize)
{
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(bufferDataSize);
	if (stagingBuffer == nullptr)
	{
		stagingBuffer = new VkManagedBuffer{ m_vkDevice };
	}
	
	uint32_t currentBufferCount = static_cast<uint32_t>(buffers.size());
	if (currentBufferCount == 0)
	{
		buffers.resize(objectCount, new VkManagedBuffer{ m_vkDevice });
	}
	else if (currentBufferCount != objectCount)
	{
		for (uint32_t i = 0; i < currentBufferCount; ++i)
			delete buffers[i];
		buffers.clear();
		buffers.resize(objectCount, new VkManagedBuffer{ m_vkDevice });
	}


	try
	{
		stagingBuffer->Build(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize);
		for (uint32_t i = 0; i < objectCount; i++)
		{
			buffers[i]->Build(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,bufferSize);
		}

	}
	catch (...)
	{
		throw;
	}
}
