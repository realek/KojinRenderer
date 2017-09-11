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

#include "Allocation.h"

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
		m_vkMainCmdPool = new VkManagedCommandPool(m_vkDevice->device(), m_vkDevice->GetQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, false, false, true));
		m_vkSwapchain = new VkManagedSwapchain(m_vkDevice->device(), m_vkDevice->GetPhysicalDeviceData(), VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		m_vkRenderpassFWD = new VkManagedRenderPass();
		m_vkRenderpassFWD->Build(m_vkDevice->device(), m_vkSwapchain->Extent(), VK_FORMAT_B8G8R8A8_UNORM, m_vkDevice->Depthformat());
		m_vkRenderpassFWD->SetFrameBufferCount(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), 1, 1, vkm_copy_color);
		m_vkPipelineFWD = new VkManagedPipeline();
		
		VkPushConstantRange rangeViewProj;
		rangeViewProj.offset = 0;
		rangeViewProj.size = 128;
		rangeViewProj.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		m_vkPipelineFWD->Build(m_vkDevice->device(),
			m_vkRenderpassFWD->renderPass(), PipelineMode::Solid,
			"shaders/vertex.vert.spv",
			"shaders/fragment.frag.spv",
			{ VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_VIEWPORT
			}, {rangeViewProj});

		m_vkRenderPassSDWProj = new VkManagedRenderPass();
		m_vkRenderPassSDWProj->Build(m_vkDevice->device(), { VkShadowmapDefaults::k_resolution,VkShadowmapDefaults::k_resolution }, VkShadowmapDefaults::k_attachmentDepthFormat);
		m_vkRenderPassSDWProj->SetFrameBufferCount(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), 1, 1, vkm_copy_depth);
		m_vkPipelineSDWProj = new VkManagedPipeline();

		m_vkPipelineSDWProj->Build(m_vkDevice->device(), m_vkRenderPassSDWProj->renderPass(), PipelineMode::ProjectedShadows,
			"shaders/vertexOffscreen.vert.spv",
			"shaders/fragmentOffscreen.frag.spv",
			{ VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_DEPTH_BIAS
			}, {rangeViewProj});

		m_vkDescriptorPool = new VkManagedDescriptorPool();
		m_vkDescriptorPool->SetDescriptorCount(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2);
		m_vkDescriptorPool->SetDescriptorCount(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);
		m_vkDescriptorPool->SetDescriptorCount(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);
		m_semaphores = new VkManagedSemaphore(m_vkDevice->device(), 2); // 1 present and 2 pass 
		m_swapChainCommandBuffers = m_vkMainCmdPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_vkSwapchain->ImageCount());
		m_miscCmdBuffer = m_vkMainCmdPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		m_renderCommandBuffer = m_vkMainCmdPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
		m_colorSampler = new VkManagedSampler(m_vkDevice->device(), VkManagedSamplerMode::COLOR_NORMALIZED_COORDINATES, 16, VK_BORDER_COLOR_INT_OPAQUE_BLACK);
		m_depthSampler = new VkManagedSampler(m_vkDevice->device(), VkManagedSamplerMode::DEPTH_NORMALIZED_COORDINATES, 0, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

		//set layouts for all swapchain images

		VkCommandBuffer misc = m_miscCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
		m_vkSwapchain->SetSwapchainImageLayouts(misc);
		m_miscCmdBuffer.End(0);
		m_miscCmdBuffer.Submit(m_vkMainCmdPool->PoolQueue()->queue, {}, {}, {}, 0);
		vkQueueWaitIdle(m_vkMainCmdPool->PoolQueue()->queue);

		//insert layered shadowmap
		m_deviceLoadedImages.insert(std::make_pair(m_layeredShadowMapIndex, VkManagedImage()));
		m_deviceLoadedResources.insert(std::make_pair(m_layeredShadowMapIndex, VkManagedImageResources()));

		//insert render target
		m_deviceLoadedImages.insert(std::make_pair(m_renderTargetIndex, VkManagedImage()));
		m_deviceLoadedResources.insert(std::make_pair(m_renderTargetIndex, VkManagedImageResources()));
		//setup render target
		{
			VkExtent3D ext = m_vkRenderpassFWD->GetExtent3D();
			const VkDevice& device = m_vkDevice->device();

			VkManagedImage& img = m_deviceLoadedImages[m_renderTargetIndex];
			VkManagedImageResources& imgRes = m_deviceLoadedResources[m_renderTargetIndex];

			VkImageCreateInfo imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, ext, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_B8G8R8A8_UNORM);
			VkResult result = vkCreateImage(device, &imageCI, nullptr, &img.image);
			assert(result == VK_SUCCESS);
			imgRes.image.set_object(img.image, device, vkDestroyImage);

			VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), img.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &img.imageMemory);
			assert(result == VK_SUCCESS);
			imgRes.imageMemory.set_object(img.imageMemory, device, vkFreeMemory);

			result = vkBindImageMemory(device, img.image, img.imageMemory, 0);
			assert(result == VK_SUCCESS);

			img.layers = 1;
			img.imageExtent = imageCI.extent;
			img.format = imageCI.format;
			img.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}
	catch(...)
	{

		throw;
	}
	

}

Vulkan::KojinRenderer::~KojinRenderer()
{
	CleanNonVulkanItems();
	m_deviceLoadedResources.clear();
	delete(m_colorSampler);
	delete(m_depthSampler);
	delete(m_uniformLightingBufferFWD);
	delete(m_uniformLightingstagingBufferFWD);
	delete(m_uniformShadowBufferFWD);
	delete(m_uniformShadowstagingBufferFWD);
	delete(m_uniformVModelDynamicBuffer);
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
	
	m_meshPartData.reserve(meshSize);
	m_meshPartTextures.reserve(meshSize);
	for(size_t i = 0; i < meshSize; ++i)
	{
		if (std::find_if(m_meshDraws.begin(), m_meshDraws.end(),
			[&meshes, &i](std::pair<const uint32_t, int> a)->bool { return a.first == meshes[i]->id; }) != m_meshDraws.end())
		{
			m_meshDraws[meshes[i]->id]++;
		}
		else
		{
			m_meshDraws.insert(std::make_pair(meshes[i]->id, 1));
		}
		m_objectCount++;
		m_meshPartData.push_back({ meshes[i]->modelMatrix, materials[i]->diffuseColor, materials[i]->specularity });
		m_meshPartTextures.push_back(materials[i]->albedo->id);
	}
}

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

void Vulkan::KojinRenderer::CleanNonVulkanItems()
{
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
	}

	MemoryAllocation::FreeAligned(m_uniformVModelDynamicBufferAlignedData);
}

void Vulkan::KojinRenderer::FreeLight(Light * light)
{
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
	m_virtualTextures.insert(std::make_pair(texture->id, texture));

	const VkDevice& device = m_vkDevice->device();

	DeviceImageResources::iterator resourceIt = m_deviceLoadedResources.insert(std::make_pair(texture->id, VkManagedImageResources())).first;
	DeviceImageList::iterator imageIt = m_deviceLoadedImages.insert(std::make_pair(texture->id, VkManagedImage())).first;

	VkManagedImageResources stagingImageResources; // will clear data when we exit scope
	VkManagedImage stagingImage;

	imageIt->second.format = colorFormat;
	imageIt->second.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	imageIt->second.layers = 1;
	stagingImage.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	stagingImage.layers = 1;

	VkImageCreateInfo imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_TRANSFER_SRC_BIT, { (uint32_t)surf->w, (uint32_t)surf->h, 1U }, 1, VK_IMAGE_TILING_LINEAR, colorFormat);
	VkResult result = vkCreateImage(device, &imageCI, nullptr, &stagingImage.image);
	assert(result == VK_SUCCESS);
	stagingImageResources.image.set_object(stagingImage.image, device, vkDestroyImage);

	VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), stagingImage.image, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &stagingImage.imageMemory);
	assert(result == VK_SUCCESS);
	stagingImageResources.imageMemory.set_object(stagingImage.imageMemory, device, vkFreeMemory);

	result = vkBindImageMemory(device, stagingImage.image, stagingImage.imageMemory, 0);
	assert(result == VK_SUCCESS);

	stagingImage.imageExtent = imageCI.extent;

	imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, stagingImage.imageExtent, 1, VK_IMAGE_TILING_OPTIMAL, colorFormat);
	result = vkCreateImage(device, &imageCI, nullptr, &imageIt->second.image);
	assert(result == VK_SUCCESS);
	resourceIt->second.image.set_object(imageIt->second.image, device, vkDestroyImage);

	imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), imageIt->second.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &imageIt->second.imageMemory);
	assert(result == VK_SUCCESS);
	resourceIt->second.imageMemory.set_object(imageIt->second.imageMemory, device, vkFreeMemory);

	result = vkBindImageMemory(device, imageIt->second.image, imageIt->second.imageMemory, 0);
	assert(result == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI = GetImageViewCreateInfo(imageIt->second.image, 1, imageCI.imageType, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	result = vkCreateImageView(device, &imageViewCI, nullptr, &imageIt->second.imageView);
	assert(result == VK_SUCCESS);
	resourceIt->second.imageView.set_object(imageIt->second.imageView, device, vkDestroyImageView);

	VkManagedCommandBuffer buffer = m_vkMainCmdPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkManagedQueue * cmdPoolQueue = m_vkMainCmdPool->PoolQueue();
	VkCommandBuffer cmdBuffer = buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
	imageIt->second.LoadData(stagingImage, m_vkDevice->device(), cmdBuffer, surf->pixels, bytesPerPixel, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	buffer.End(0);
	buffer.Submit(cmdPoolQueue->queue, {}, {}, {});
	SDL_free(surf);
	vkQueueWaitIdle(cmdPoolQueue->queue);

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

	const VkDevice& device = m_vkDevice->device();

	DeviceImageResources::iterator resourceIt = m_deviceLoadedResources.insert(std::make_pair(m_whiteTexture->id, VkManagedImageResources())).first;
	DeviceImageList::iterator imageIt = m_deviceLoadedImages.insert(std::make_pair(m_whiteTexture->id, VkManagedImage())).first;

	VkManagedImageResources stagingImageResources; // will clear data when we exit scope
	VkManagedImage stagingImage;

	imageIt->second.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageIt->second.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	imageIt->second.layers = 1;
	stagingImage.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	stagingImage.layers = 1;

	VkImageCreateInfo imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_TRANSFER_SRC_BIT, { w, h, 1U }, 1, VK_IMAGE_TILING_LINEAR, VK_FORMAT_R8G8B8A8_UNORM);
	VkResult result = vkCreateImage(device, &imageCI, nullptr, &stagingImage.image);
	assert(result == VK_SUCCESS);
	stagingImageResources.image.set_object(stagingImage.image, device, vkDestroyImage);

	VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), stagingImage.image, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &stagingImage.imageMemory);
	assert(result == VK_SUCCESS);
	stagingImageResources.imageMemory.set_object(stagingImage.imageMemory, device, vkFreeMemory);

	result = vkBindImageMemory(device, stagingImage.image, stagingImage.imageMemory, 0);
	assert(result == VK_SUCCESS);

	stagingImage.imageExtent = imageCI.extent;

	imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, stagingImage.imageExtent, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_R8G8B8A8_UNORM);
	result = vkCreateImage(device, &imageCI, nullptr, &imageIt->second.image);
	assert(result == VK_SUCCESS);
	resourceIt->second.image.set_object(imageIt->second.image, device, vkDestroyImage);

	imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), imageIt->second.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &imageIt->second.imageMemory);
	assert(result == VK_SUCCESS);
	resourceIt->second.imageMemory.set_object(imageIt->second.imageMemory, device, vkFreeMemory);

	result = vkBindImageMemory(device, imageIt->second.image, imageIt->second.imageMemory, 0);
	assert(result == VK_SUCCESS);

	VkImageViewCreateInfo imageViewCI = GetImageViewCreateInfo(imageIt->second.image, 1, imageCI.imageType, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	result = vkCreateImageView(device, &imageViewCI, nullptr, &imageIt->second.imageView);
	assert(result == VK_SUCCESS);
	resourceIt->second.imageView.set_object(imageIt->second.imageView, device, vkDestroyImageView);

	VkManagedCommandBuffer buffer = m_vkMainCmdPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	VkManagedQueue * cmdPoolQueue = m_vkMainCmdPool->PoolQueue();
	VkCommandBuffer cmdBuffer = buffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
	imageIt->second.LoadData(stagingImage, m_vkDevice->device(), cmdBuffer, pixels.data(), 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	buffer.End(0);
	buffer.Submit(cmdPoolQueue->queue, {}, {}, {});
	vkQueueWaitIdle(cmdPoolQueue->queue);

	return m_whiteTexture;

	//auto image = std::make_shared<VkManagedImage>(m_vkDevice);
	//m_deviceLoadedTextures.insert(std::make_pair(m_whiteTexture->id, image));
	//image->Build({w,h }, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	//VkManagedCommandBuffer cmdbuff = m_vkMainCmdPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	//VkManagedQueue * cmdPoolQueue = m_vkMainCmdPool->PoolQueue();
	//image->LoadData(m_vkDevice->device(), &cmdbuff, pixels.data(), 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	//cmdbuff.Free();

	return m_whiteTexture;
}
void Vulkan::KojinRenderer::FreeTexture(Texture * tex) 
{
	assert(tex != nullptr);
	assert(tex->id != m_whiteTexture->id); // can't delete internal texture
	assert(m_deviceLoadedResources.count(tex->id) != 0);
	m_deviceLoadedResources.erase(tex->id);
	m_deviceLoadedImages.erase(tex->id);
	delete tex;
}

void Vulkan::KojinRenderer::Render()
{
	//acquire image
	uint32_t scImage = 0;
	m_vkSwapchain->AcquireNextImage(m_vkDevice->device(), &scImage, m_semaphores->Last());

	bool rebuild = false;
	if(m_objectCount != m_objectCountOld)
	{
		m_objectCountOld = m_objectCount;
		this->UpdateInternalMesh(m_vkMainCmdPool, 
			Mesh::m_iMeshVertices.data(), static_cast<uint32_t>(Mesh::m_iMeshVertices.size()), 
			Mesh::m_iMeshIndices.data(), static_cast<uint32_t>(Mesh::m_iMeshIndices.size()));
		
		if(m_vkDescriptorPool->Size() < (uint32_t)m_objectCount*3)
		{
			const VkDevice& device = m_vkDevice->device();
			m_vkDescriptorPool->BuildPool(device, m_objectCount*3);
			m_vDescriptorSetFWD = m_vkDescriptorPool->AllocateDescriptorSet(device, m_objectCount, m_vkPipelineFWD->GetVertexLayout());
			m_fDescriptorSetFWD = m_vkDescriptorPool->AllocateDescriptorSet(device, m_objectCount, m_vkPipelineFWD->GetFragmentLayout());
			m_vDescriptorSetSDWProj = m_vkDescriptorPool->AllocateDescriptorSet(device, m_objectCount, m_vkPipelineSDWProj->GetVertexLayout());
			CreateDynamicUniformBuffer(m_uniformVModelDynamicBuffer, m_objectCount, sizeof(mat4_vec4_float_container));
			CreateUniformBufferPair(m_uniformLightingstagingBufferFWD, m_uniformLightingBufferFWD, sizeof(vec4x4x6_vec4_container)); //one lighting buffer
			CreateUniformBufferPair(m_uniformShadowstagingBufferFWD, m_uniformShadowBufferFWD, sizeof(mat4x6_container)); //one lighting buffer
			//make sure to delete all buffers on clean function call
		
		}

		rebuild = true;
	}


	UpdateShadowmapLayers();

	//update model uniform buffers
	m_miscCmdBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	{
		VkCommandBuffer uniformCmdBuffer = m_miscCmdBuffer.Buffer();

		//render target & shadowmap layout check
		{
			VkManagedImage& shMap = m_deviceLoadedImages[m_layeredShadowMapIndex];
			if (shMap.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				shMap.SetLayout(uniformCmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0, shMap.layers, VK_QUEUE_FAMILY_IGNORED);
			}

			VkManagedImage& rt = m_deviceLoadedImages[m_renderTargetIndex];
			if (rt.layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				rt.SetLayout(uniformCmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, rt.layers, VK_QUEUE_FAMILY_IGNORED);
			}
		}


		update_dynamic_uniformBuffer(m_uniformVModelDynamicBuffer, m_meshPartData);

		vec4x4x6_vec4_container light_data = {};
		mat4x6_container shadow_data = {};
		create_lighting_data_forward(light_data, shadow_data);
		staged_update_uniformBuffer(uniformCmdBuffer, m_uniformLightingstagingBufferFWD, m_uniformLightingBufferFWD, &light_data, sizeof(light_data));
		staged_update_uniformBuffer(uniformCmdBuffer, m_uniformShadowstagingBufferFWD, m_uniformShadowBufferFWD, &shadow_data, sizeof(shadow_data));

	}
	m_miscCmdBuffer.End();
	m_miscCmdBuffer.Submit(m_vkMainCmdPool->PoolQueue()->queue, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_semaphores->Next()}, { m_semaphores->Last()});

	std::vector<VkIndexedDraw> indexdraws;
	indexdraws.resize(m_objectCount);
	{
		uint32_t objIndex = 0;
		for(std::pair<const int,int>& pair : m_meshDraws)
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

	//write descriptors here!
	for (int oc = 0; oc < m_objectCountOld; ++oc)
	{
		WriteShadowDescriptorSet(oc);
		WriteDescriptors(oc);
	}

	//set states for the forward render pipeline
	VkDynamicStatesBlock states;
	states.viewports.resize(1);
	states.scissors.resize(1);
	states.hasViewport = VK_TRUE;
	states.hasScissor = VK_TRUE;
	//clear values
	std::vector<VkClearValue> clearValues;
	clearValues.resize(2);
	clearValues[0].color = { 0,0.15f,0.1f,1.0f };
	clearValues[1].depthStencil = { 1, 0 };

	m_renderCommandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	VkCommandBuffer cmds = m_renderCommandBuffer.Buffer();
	//shadow pass cmds
	{


		states.hasDepthBias = VK_TRUE;
		uint32_t i = 0;

		for (std::pair<uint32_t, Light*> l : m_lights)
		{
			states.scissors[0].extent = m_vkRenderPassSDWProj->GetExtent();
			states.scissors[0].offset.x = 0;
			states.scissors[0].offset.y = 0;
			states.viewports[0].maxDepth = 1.0f;
			states.viewports[0].height = (float)states.scissors[0].extent.height;
			states.viewports[0].width = (float)states.scissors[0].extent.width;
			states.viewports[0].minDepth = 0;
			states.viewports[0].maxDepth = 1.0f;
			states.viewports[0].x = 0;
			states.viewports[0].y = 0;
			states.depthBias.constDepth = VkShadowmapDefaults::k_depthBias;
			states.depthBias.depthSlope = VkShadowmapDefaults::k_depthBiasSlope;
			states.depthBias.depthClamp = 0.0f;

			m_vkRenderPassSDWProj->SetPipeline(m_vkPipelineSDWProj, states, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS);
			m_vkRenderPassSDWProj->PreRecordData(cmds, 0); //we have 1 framebuffer
			std::vector<VkPushConstant> constants(2);

			constants[0].data = &l.second->GetLightViewMatrix();
			constants[0].offset = 0;
			constants[0].size = sizeof(glm::mat4);
			constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			constants[1].data = &l.second->GetLightProjectionMatrix();
			constants[1].offset = constants[0].size;
			constants[1].size = sizeof(glm::mat4);
			constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			std::vector<VkManagedDescriptorSet*> sets{ &m_vDescriptorSetSDWProj };
			m_vkRenderPassSDWProj->Record(VkShadowmapDefaults::k_clearValues, sets, constants, m_meshIndexData, m_meshVertexData, indexdraws);

			//copy pass result
			VkManagedImage passDepth = m_vkRenderPassSDWProj->GetAttachment(0, VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			VkManagedImage& shMap = m_deviceLoadedImages[m_layeredShadowMapIndex];

			VkImageSubresourceLayers dstRes;
			dstRes.aspectMask = shMap.aspect;
			dstRes.baseArrayLayer = i;
			dstRes.mipLevel = 0;
			dstRes.layerCount = 1;

			VkExtent3D copyExtent = {};
			copyExtent.depth = 1;
			copyExtent.height = VkShadowmapDefaults::k_resolution;
			copyExtent.width = copyExtent.height;
			passDepth.Copy(cmds, copyExtent, &shMap, m_vkPresentQueue->familyIndex, { 0,0,0 }, { 0,0,0 }, { (0),(0),(0),(1) }, dstRes); //needs per layer setup here
			i++;
		}

	}
	//fwd pass cmds
	{
		states.hasDepthBias = VK_FALSE;
		VkManagedImage renderTarget = m_deviceLoadedImages[m_renderTargetIndex];

		for (std::pair<uint32_t, Camera*> camera : m_cameras)
		{
			states.viewports[0] = camera.second->m_viewPort;
			states.scissors[0] = camera.second->m_scissor;

			m_vkRenderpassFWD->SetPipeline(m_vkPipelineFWD, states, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS);
			m_vkRenderpassFWD->PreRecordData(cmds, 0); //we have 1 framebuffer
			std::vector<VkPushConstant> constants(2);
			constants[0].data = &camera.second->m_viewMatrix;
			constants[0].offset = 0;
			constants[0].size = sizeof(camera.second->m_viewMatrix);
			constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			constants[1].data = &camera.second->m_projectionMatrix;
			constants[1].offset = constants[0].size;
			constants[1].size = sizeof(camera.second->m_projectionMatrix);
			constants[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			m_vkRenderpassFWD->Record(clearValues, { &m_vDescriptorSetFWD, &m_fDescriptorSetFWD }, constants, m_meshIndexData, m_meshVertexData, indexdraws);

			//copy pass result

			VkManagedImage passColor = m_vkRenderpassFWD->GetAttachment(0, VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

			VkExtent3D copyExtent = {};
			copyExtent.depth = 1;
			copyExtent.width = (uint32_t)camera.second->m_viewPort.width;
			copyExtent.height = (uint32_t)camera.second->m_viewPort.height;

			VkOffset3D copyOffset = {};
			copyOffset.z = 0; // same depth
			copyOffset.x = camera.second->m_scissor.offset.x;
			copyOffset.y = camera.second->m_scissor.offset.y;

			passColor.Copy(cmds, copyExtent, &renderTarget, VK_QUEUE_FAMILY_IGNORED, copyOffset, copyOffset);
		}
	}
	m_renderCommandBuffer.End();
	m_renderCommandBuffer.Submit(m_vkPresentQueue->queue, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_semaphores->Next() }, { m_semaphores->Last() });

	m_swapChainCommandBuffers.Begin(VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
	{
		VkManagedImage renderTarget = m_deviceLoadedImages[m_renderTargetIndex];
		for (uint32_t cmdIndex = 0; cmdIndex < m_swapChainCommandBuffers.Size(); ++cmdIndex)
		{
			VkManagedImage* scImage = m_vkSwapchain->SwapchainImage(cmdIndex);
			renderTarget.Copy(m_swapChainCommandBuffers.Buffer(cmdIndex), renderTarget.imageExtent, scImage);
		}
	}
	m_swapChainCommandBuffers.End();
	m_swapChainCommandBuffers.Submit(m_vkPresentQueue->queue, { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, { m_semaphores->Next() }, { m_semaphores->Last() });
	
	//present
	m_vkSwapchain->PresentCurrentImage(&scImage, m_vkPresentQueue, { m_semaphores->Last()}); // pass waiting semaphores

	//clean
	m_objectCount = 0;
	m_meshDraws.clear();

	m_meshPartData.resize(0);
	m_meshPartTextures.resize(0);

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
		m_meshIndexData = new VkManagedBuffer();
		m_meshVertexData = new VkManagedBuffer();
	}
	//create vertex and index buffers for the mesh
	//declare staging buffers
	VkManagedBuffer vertexStagingBuffer;
	VkManagedBuffer indiceStagingBuffer;

	//create staging buffers
	vertexStagingBuffer.Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexSize, 1, VK_FALSE);
	indiceStagingBuffer.Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indiceSize, 1, VK_FALSE);

	//copy data into staging buffers
	vertexStagingBuffer.Write(m_vkDevice->device(), 0, 0, (size_t)vertexStagingBuffer.Size(), vertexData);
	indiceStagingBuffer.Write(m_vkDevice->device(), 0, 0, (size_t)indiceStagingBuffer.Size(), indiceData);

	//create and load normal buffers
	m_meshVertexData->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexSize, 1, VK_FALSE);
	m_meshIndexData->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indiceSize, 1, VK_FALSE);

	VkManagedCommandBuffer cmdBuff = commandPool->CreateCommandBuffer(m_vkDevice->device(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	cmdBuff.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);
	VkCommandBuffer buffer = cmdBuff.Buffer();
	vertexStagingBuffer.CopyTo(buffer, m_meshVertexData, 0, 0, vertexSize);
	indiceStagingBuffer.CopyTo(buffer, m_meshIndexData, 0, 0, indiceSize);
	cmdBuff.End(0);
	cmdBuff.Submit(commandPool->PoolQueue()->queue, {}, {}, {}, 0);
	m_vkDevice->WaitForIdle();
	commandPool->Free(&cmdBuff, m_vkDevice->device());
}

template<typename T>
void Vulkan::KojinRenderer::update_dynamic_uniformBuffer(VkManagedBuffer * dyn_buffer, std::vector<T>& data) 
{
	VkDeviceSize alignment = dyn_buffer->Alignment();
	assert(alignment != 0); //if alignment is 0 then the buffer provided isn't dynamic //yes yes ugly

	if(m_uniformVModelDynamicBufferAlignedData==nullptr)
		m_uniformVModelDynamicBufferAlignedData = MemoryAllocation::AllocateAligned((size_t)dyn_buffer->Size(), (size_t)alignment);

	T* alignedData = (T*)m_uniformVModelDynamicBufferAlignedData;
	for (size_t i = 0; i < data.size(); ++i) 
	{
		T* p = (T*)((VkDeviceSize)alignedData + (i*alignment));
		*p = data[i];
	}

	dyn_buffer->Write(m_vkDevice->device(), 0, 0, data.size()*alignment, alignedData);
	//MemoryAllocation::FreeAligned(alignedData);
}
void Vulkan::KojinRenderer::staged_update_uniformBuffer(VkCommandBuffer recordBuffer, VkManagedBuffer * stagingBuffer, VkManagedBuffer * targetBuffer, void * data, size_t dataSize) 
{
	stagingBuffer->Write(m_vkDevice->device(), 0, 0, dataSize, data);
	try
	{
		stagingBuffer->CopyTo(recordBuffer, targetBuffer, 0, 0, dataSize);
	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::KojinRenderer::create_lighting_data_forward(vec4x4x6_vec4_container& lightingUBO, mat4x6_container& shadowUBO)
{
	lightingUBO.va = glm::vec4(0.1, 0.1, 0.1, 0.1);
	uint32_t i = 0;
	for (std::pair<const uint32_t, Light*>& l : m_lights)
	{
		lightingUBO.vec4x4x6[i].va = l.second->diffuseColor;
		lightingUBO.vec4x4x6[i].vc = l.second->GetLightForward();
		lightingUBO.vec4x4x6[i].vb = glm::vec4(l.second->m_position, 1.0f);
		lightingUBO.vec4x4x6[i].vb.x *= -1;
		lightingUBO.vec4x4x6[i].vd = {};
		lightingUBO.vec4x4x6[i].vd[0] = (float)l.second->GetType();
		lightingUBO.vec4x4x6[i].vd[1] = l.second->intensity;
		lightingUBO.vec4x4x6[i].vd[2] = l.second->range;
		lightingUBO.vec4x4x6[i].vd[3] = l.second->angle;
		shadowUBO.matrices[i] = VkShadowmapDefaults::k_shadowBiasMatrix * (l.second->GetLightProjectionMatrix() * l.second->GetLightViewMatrix());
		i++;
	}
}

void Vulkan::KojinRenderer::WriteShadowDescriptorSet(uint32_t objectIndex) 
{
	VkDescriptorSet set = m_vDescriptorSetSDWProj.Set(objectIndex);

	VkDescriptorBufferInfo vertexModelBuffer = {};
	vertexModelBuffer.buffer = m_uniformVModelDynamicBuffer->buffer();
	vertexModelBuffer.offset = 0;
	vertexModelBuffer.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet vertexWrites = {};
	{
		vertexWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexWrites.dstSet = set;
		vertexWrites.dstBinding = 0;
		vertexWrites.dstArrayElement = 0;
		vertexWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		vertexWrites.descriptorCount = 1;
		vertexWrites.pBufferInfo = &vertexModelBuffer;
	}

	m_vDescriptorSetSDWProj.uniformDynamicOffsetCount = 1;
	m_vDescriptorSetSDWProj.uniformDynamicAlignment = m_uniformVModelDynamicBuffer->Alignment();
	vkUpdateDescriptorSets(m_vkDevice->device(), 1, &vertexWrites, 0, nullptr);
}

void Vulkan::KojinRenderer::WriteDescriptors(uint32_t objIndex)
{
	//VERTEX
	VkDescriptorSet set = m_vDescriptorSetFWD.Set(objIndex);

	VkDescriptorBufferInfo vertexModelBuffer = {};
	vertexModelBuffer.buffer = m_uniformVModelDynamicBuffer->buffer();
	vertexModelBuffer.offset = 0;
	vertexModelBuffer.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo vertexLightingBuffer = {};
	vertexLightingBuffer.buffer = m_uniformLightingBufferFWD->buffer();
	vertexLightingBuffer.offset = 0;
	vertexLightingBuffer.range = VK_WHOLE_SIZE;

	std::vector<VkWriteDescriptorSet> vertexWrites(2);
	{
		vertexWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexWrites[0].dstSet = set;
		vertexWrites[0].dstBinding = 0;
		vertexWrites[0].dstArrayElement = 0;
		vertexWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		vertexWrites[0].descriptorCount = 1;
		vertexWrites[0].pBufferInfo = &vertexModelBuffer;

		vertexWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexWrites[1].dstSet = set;
		vertexWrites[1].dstBinding = 1;
		vertexWrites[1].dstArrayElement = 0;
		vertexWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vertexWrites[1].descriptorCount = 1;
		vertexWrites[1].pBufferInfo = &vertexLightingBuffer;
	}

	m_vDescriptorSetFWD.uniformDynamicOffsetCount = 1;
	m_vDescriptorSetFWD.uniformDynamicAlignment = m_uniformVModelDynamicBuffer->Alignment();
	vkUpdateDescriptorSets(m_vkDevice->device(), (uint32_t)vertexWrites.size(), vertexWrites.data(), 0, nullptr);
	
	//FRAGMENT
	set = m_fDescriptorSetFWD.Set(objIndex);

	VkDescriptorImageInfo diffuseTextureInfo = {};
	diffuseTextureInfo.imageLayout = m_deviceLoadedImages[m_meshPartTextures[objIndex]].layout;
	diffuseTextureInfo.imageView = m_deviceLoadedImages[m_meshPartTextures[objIndex]].imageView;
	diffuseTextureInfo.sampler = m_colorSampler->sampler();

	VkDescriptorImageInfo shadowMaptexture = {};
	shadowMaptexture.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	shadowMaptexture.imageView = m_deviceLoadedImages[m_layeredShadowMapIndex].imageView;
	shadowMaptexture.sampler = m_depthSampler->sampler();

	VkDescriptorBufferInfo fragmentShadowBuffer = {};
	fragmentShadowBuffer.buffer = m_uniformShadowBufferFWD->buffer();
	fragmentShadowBuffer.offset = 0;
	fragmentShadowBuffer.range = VK_WHOLE_SIZE;

	std::vector<VkWriteDescriptorSet> fragmentDescriptorWrites(3);
	{
		fragmentDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[0].dstSet = set;
		fragmentDescriptorWrites[0].dstBinding = 0;
		fragmentDescriptorWrites[0].dstArrayElement = 0;
		fragmentDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fragmentDescriptorWrites[0].descriptorCount = 1;
		fragmentDescriptorWrites[0].pImageInfo = &diffuseTextureInfo;

		fragmentDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[1].dstSet = set;
		fragmentDescriptorWrites[1].dstBinding = 1;
		fragmentDescriptorWrites[1].dstArrayElement = 0;
		fragmentDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fragmentDescriptorWrites[1].descriptorCount = 1;
		fragmentDescriptorWrites[1].pImageInfo = &shadowMaptexture;

		fragmentDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		fragmentDescriptorWrites[2].dstSet = set;
		fragmentDescriptorWrites[2].dstBinding = 2;
		fragmentDescriptorWrites[2].dstArrayElement = 0;
		fragmentDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		fragmentDescriptorWrites[2].descriptorCount = 1;
		fragmentDescriptorWrites[2].pBufferInfo = &fragmentShadowBuffer;
	}

	vkUpdateDescriptorSets(m_vkDevice->device(), (uint32_t)(fragmentDescriptorWrites.size()), fragmentDescriptorWrites.data(), 0, nullptr);
}

bool Vulkan::KojinRenderer::UpdateShadowmapLayers()
{
	size_t lightCount = m_lights.size();
	VkManagedImage& shMap = m_deviceLoadedImages[m_layeredShadowMapIndex];
	VkManagedImageResources& shMapResouces = m_deviceLoadedResources[m_layeredShadowMapIndex];
	if(lightCount != shMap.layers || shMapResouces.image.object() == VK_NULL_HANDLE)
	{
		VkExtent3D ext;
		ext.height = VkShadowmapDefaults::k_resolution;
		ext.width = VkShadowmapDefaults::k_resolution;
		ext.depth = 1U;
		const VkDevice& device = m_vkDevice->device();

		VkImageCreateInfo imageCI = GetImage2DCreateInfo(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, ext, lightCount, VK_IMAGE_TILING_OPTIMAL, VkShadowmapDefaults::k_attachmentDepthFormat);
		VkResult result = vkCreateImage(device, &imageCI, nullptr, &shMap.image);
		assert(result == VK_SUCCESS);
		shMapResouces.image.set_object(shMap.image, device, vkDestroyImage);

		VkMemoryAllocateInfo imageMemoryAI = GetImageMemoryAllocateInfo(device, m_vkDevice->PhysicalDevice(), shMap.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		result = vkAllocateMemory(device, &imageMemoryAI, nullptr, &shMap.imageMemory);
		assert(result == VK_SUCCESS);
		shMapResouces.imageMemory.set_object(shMap.imageMemory, device, vkFreeMemory);

		result = vkBindImageMemory(device, shMap.image, shMap.imageMemory, 0);
		assert(result == VK_SUCCESS);

		VkImageViewCreateInfo imageViewCI = GetImageViewCreateInfo(shMap.image, lightCount, imageCI.imageType, imageCI.format, VK_IMAGE_ASPECT_DEPTH_BIT, 0, true); // is always array
		result = vkCreateImageView(device, &imageViewCI, nullptr, &shMap.imageView);
		assert(result == VK_SUCCESS);
		shMapResouces.imageView.set_object(shMap.imageView, device, vkDestroyImageView);

		shMap.layers = lightCount;
		shMap.imageExtent = imageCI.extent;
		shMap.format = imageCI.format;
		shMap.aspect = imageViewCI.subresourceRange.aspectMask;

		//m_layeredShadowMap->Build(ext, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		//	(uint32_t)lightCount, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_D32_SFLOAT, 
		//	VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		return true;
	}
	else
	{
		return false;
	}
}

void Vulkan::KojinRenderer::CreateDynamicUniformBuffer(VkManagedBuffer *& dyn_buffer, uint32_t objectCount, uint32_t bufferDataSize) 
{
	if (dyn_buffer == nullptr)
	{
		dyn_buffer = new VkManagedBuffer();
	}

	dyn_buffer->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufferDataSize, objectCount, VK_TRUE);
}

void Vulkan::KojinRenderer::CreateUniformBufferSet(VkManagedBuffer *& stagingBuffer, std::vector<VkManagedBuffer*>& buffers, uint32_t objectCount, uint32_t bufferDataSize)
{
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(bufferDataSize);
	if (stagingBuffer == nullptr)
	{
		stagingBuffer = new VkManagedBuffer();
	}
	
	uint32_t currentBufferCount = static_cast<uint32_t>(buffers.size());
	if (currentBufferCount == 0)
	{
		buffers.resize(objectCount, new VkManagedBuffer());
	}
	else if (currentBufferCount != objectCount)
	{
		for (uint32_t i = 0; i < currentBufferCount; ++i)
			delete buffers[i];
		buffers.clear();
		buffers.resize(objectCount, new VkManagedBuffer());
	}


	try
	{
		stagingBuffer->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, 1, VK_FALSE);
		for (uint32_t i = 0; i < objectCount; i++)
		{
			buffers[i]->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,bufferSize, 1, VK_FALSE);
		}

	}
	catch (...)
	{
		throw;
	}
}

void Vulkan::KojinRenderer::CreateUniformBufferPair(VkManagedBuffer *& stagingBuffer, VkManagedBuffer *& buffer, uint32_t bufferDataSize)
{
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(bufferDataSize);
	if (stagingBuffer == nullptr)
	{
		stagingBuffer = new VkManagedBuffer();
	}

	if (buffer == nullptr)
	{
		buffer = new VkManagedBuffer();
	}


	try
	{
		stagingBuffer->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, bufferSize, 1, VK_FALSE);
		buffer->Build(m_vkDevice->device(), m_vkDevice->PhysicalDevice(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferSize, 1, VK_FALSE);

	}
	catch (...)
	{
		throw;
	}
}