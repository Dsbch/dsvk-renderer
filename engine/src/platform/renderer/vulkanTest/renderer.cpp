#include <pch.h>

#define VMA_IMPLEMENTATION
#include "renderer.h"

#include "platform/renderer/vertex.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <meshoptimizer.h>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>
#include "renderer.h"

static bool loadMeshFromGLTF(const std::filesystem::path& path,
	std::vector<engine::vertex>& outVertices,
	std::vector<uint32_t>& outIndices)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) return false;

	size_t size = file.tellg();
	file.seekg(0);
	std::vector<uint8_t> fileData(size);
	file.read(reinterpret_cast<char*>(fileData.data()), size);

	cgltf_options options{};
	cgltf_data* data = nullptr;

	if (cgltf_parse(&options, fileData.data(), fileData.size(), &data) != cgltf_result_success)
		return false;

	if (cgltf_load_buffers(&options, data, path.parent_path().string().c_str()) != cgltf_result_success) {
		cgltf_free(data);
		return false;
	}

	outVertices.clear();
	outIndices.clear();

	// --- Helper functions ---
	auto getNodeLocalTransform = [](const cgltf_node* node) -> glm::mat4 {
		if (node->has_matrix) {
			glm::mat4 m;
			memcpy(&m[0][0], node->matrix, sizeof(float) * 16);
			return m;
		}
		else {
			glm::vec3 translation(0.0f);
			if (node->translation) translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);

			glm::quat rotation(1, 0, 0, 0);
			if (node->rotation) rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);

			glm::vec3 scale(1.0f);
			if (node->scale) scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);

			return glm::translate(glm::mat4(1.0f), translation)
				* glm::mat4_cast(rotation)
				* glm::scale(glm::mat4(1.0f), scale);
		}
		};

	std::function<glm::mat4(const cgltf_node*)> getNodeWorldTransform = [&](const cgltf_node* node) -> glm::mat4 {
		if (!node->parent) return getNodeLocalTransform(node);
		return getNodeWorldTransform(node->parent) * getNodeLocalTransform(node);
		};

	auto processPrimitive = [&](const cgltf_primitive& prim, const glm::mat4& transform) {
		if (prim.type != cgltf_primitive_type_triangles) return;

		const cgltf_accessor* positionAccessor = nullptr;
		const cgltf_accessor* normalAccessor = nullptr;
		const cgltf_accessor* texcoordAccessor = nullptr;

		for (size_t ai = 0; ai < prim.attributes_count; ++ai) {
			const cgltf_attribute& attr = prim.attributes[ai];
			switch (attr.type) {
			case cgltf_attribute_type_position: positionAccessor = attr.data; break;
			case cgltf_attribute_type_normal: normalAccessor = attr.data; break;
			case cgltf_attribute_type_texcoord: texcoordAccessor = attr.data; break;
			default: break;
			}
		}

		if (!positionAccessor || positionAccessor->component_type != cgltf_component_type_r_32f || positionAccessor->type != cgltf_type_vec3)
			return;

		uint32_t baseIndex = static_cast<uint32_t>(outVertices.size());

		// --- Vertices ---
		for (size_t i = 0; i < positionAccessor->count; ++i) {
			engine::vertex v{};

			float pos[3]{};
			cgltf_accessor_read_float(positionAccessor, i, pos, 3);
			glm::vec4 localPos(pos[0], pos[1], pos[2], 1.0f);
			v.position = glm::vec3(transform * localPos);

			if (normalAccessor) {
				float norm[3]{};
				cgltf_accessor_read_float(normalAccessor, i, norm, 3);
				glm::vec3 n(norm[0], norm[1], norm[2]);
				v.normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(transform))) * n);
			}

			if (texcoordAccessor) {
				float uv[2]{};
				cgltf_accessor_read_float(texcoordAccessor, i, uv, 2);
				v.textureCoords = glm::vec2(uv[0], uv[1]);
			}

			outVertices.push_back(v);
		}

		// --- Indices ---
		if (prim.indices) {
			const cgltf_accessor* indexAccessor = prim.indices;
			const uint8_t* bufferStart = reinterpret_cast<const uint8_t*>(
				indexAccessor->buffer_view->buffer->data) +
				indexAccessor->buffer_view->offset + indexAccessor->offset;

			size_t stride = indexAccessor->stride ? indexAccessor->stride :
				(indexAccessor->component_type == cgltf_component_type_r_16u ? 2 :
					indexAccessor->component_type == cgltf_component_type_r_32u ? 4 : 1);

			for (size_t i = 0; i < indexAccessor->count; ++i) {
				const uint8_t* elem = bufferStart + i * stride;
				uint32_t index = 0;

				switch (indexAccessor->component_type) {
				case cgltf_component_type_r_16u: index = *reinterpret_cast<const uint16_t*>(elem); break;
				case cgltf_component_type_r_32u: index = *reinterpret_cast<const uint32_t*>(elem); break;
				case cgltf_component_type_r_8u:  index = *reinterpret_cast<const uint8_t*>(elem); break;
				default: continue;
				}

				outIndices.push_back(baseIndex + index);
			}
		}
		};

	// --- Main loop: iterate nodes ---
	for (size_t ni = 0; ni < data->nodes_count; ++ni) {
		const cgltf_node* node = &data->nodes[ni];
		if (!node->mesh) continue;

		glm::mat4 transform = getNodeWorldTransform(node);
		const cgltf_mesh& mesh = *node->mesh;

		for (size_t pri = 0; pri < mesh.primitives_count; ++pri) {
			processPrimitive(mesh.primitives[pri], transform);
		}
	}

	cgltf_free(data);
	return true;
}

struct pushConstants
{
	glm::mat4 viewProjection;
};

static size_t meshletsCount = 0;

namespace vktest
{
	PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;

	void vulkanRenderer::init(engine::window* window)
	{
		init_vulkan(window);
		if (mErr)
			return;

		init_immediate_submit();
		if (mErr)
			return;

		initMesh();
		if (mErr)
			return;

		init_swapchain(window->getWidth(), window->getHeight());
		if (mErr)
			return;

		initTextures();
		if (mErr)
			return;

		init_descriptors();
		if (mErr)
			return;

		init_pipelines();
		if (mErr)
			return;
	}

	void vulkanRenderer::init_vulkan(engine::window* window)
	{
		vkb::InstanceBuilder builder;

		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("Example Vulkan Application")
#ifdef DEBUG
			.request_validation_layers(true)
			.set_debug_callback(debugCallback)
#endif // DEBUG
			.require_api_version(1, 3, 0)
			.build();
		if (!inst_ret)
		{
			mErr = { inst_ret.error().message() };
			LOGERROR(mErr.err());
			return;
		}

		vkb::Instance vkb_inst = inst_ret.value();

		//grab the instance 
		_instance = vkb_inst.instance;
		_debug_messenger = vkb_inst.debug_messenger;

		//< init_instance
		// 
		//> init_device
		// create surface.
		auto surfaceResult = window->makeVulkunSurface(_instance);
		if (!surfaceResult)
		{
			mErr = surfaceResult.err();
			return;
		}

		_surface = surfaceResult.value();

		// Print all gpus.
		printGPU();

		VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures{};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		multiviewFeatures.multiview = VK_TRUE; // enable base multiview
		multiviewFeatures.pNext = nullptr;

		VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures{};
		shadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		shadingRateFeatures.primitiveFragmentShadingRate = VK_TRUE;
		shadingRateFeatures.pNext = &multiviewFeatures;

		VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
		meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		meshShaderFeatures.meshShader = VK_TRUE;
		meshShaderFeatures.taskShader = VK_TRUE; // if using task shader
		meshShaderFeatures.multiviewMeshShader = VK_TRUE;
		meshShaderFeatures.primitiveFragmentShadingRateMeshShader = VK_TRUE;
		meshShaderFeatures.pNext = &shadingRateFeatures;

		// Chain to Vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features13{};
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;
		features13.pNext = &meshShaderFeatures;

		// Vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.bufferDeviceAddress = VK_TRUE;
		features12.descriptorIndexing = VK_TRUE;
		features12.runtimeDescriptorArray = VK_TRUE;
		features12.pNext = &features13;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto selectedRes = selector
			.set_minimum_version(1, 3)
			.set_required_features_12(features12)
			.set_required_features(deviceFeatures)
			.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			.set_surface(_surface)
			.select();
		if (!selectedRes)
		{
			mErr = { selectedRes.error().message() };
			LOGERROR(mErr.err());
			return;
		}

		vkb::PhysicalDevice physicalDevice = selectedRes.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		_device = vkbDevice.device;
		_chosenGPU = physicalDevice.physical_device;
		//< init_device

		//> init_queue
		// use vkbootstrap to get a Graphics queue
		_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		//< init_queue

		 // initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _chosenGPU;
		allocatorInfo.device = _device;
		allocatorInfo.instance = _instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		auto vmaResult = vmaCreateAllocator(&allocatorInfo, &_allocator);
		if (vmaResult != VK_SUCCESS)
		{
			mErr = { vkResultToStr(vmaResult) };
			return;
		}

		loadExtensions();

		_mainDeletionQueue.push_function([&]() {
			vmaDestroyAllocator(_allocator);
			});
	}

	void vulkanRenderer::init_immediate_submit()
	{
		// init immidiate submit.
		mErr = mImmediateSubmit.init(_device, _graphicsQueue, _graphicsQueueFamily);
		if (mErr)
			return;

		_mainDeletionQueue.push_function([&] {
			mImmediateSubmit.destroy();
			});
	}

	void vulkanRenderer::init_swapchain(uint32_t width, uint32_t height)
	{
		mSwapChain.init(_allocator, _device, _surface, _chosenGPU);

		mErr = mSwapChain.build(width, height, _graphicsQueueFamily);
		if (mErr)
			return;

		//add to deletion queues
		_mainDeletionQueue.push_function([=]() {
			mSwapChain.destroy();
			});
	}

	void vulkanRenderer::init_descriptors()
	{
		mErr = mDescriptorSetCompute.init(_device, _chosenGPU);
		if (mErr)
			return;

		mErr = mDescriptorSetMesh.init(_device, _chosenGPU);
		if (mErr)
			return;

		set_trinagle_descriptor_bindings();
		set_compute_descriptors();

		_mainDeletionQueue.push_function([&]() { mDescriptorSetCompute.destroy(); });
		_mainDeletionQueue.push_function([&]() { mDescriptorSetMesh.destroy(); });
		_mainDeletionQueue.push_function([&]() { descriptorSet::destroyPool(); });
	}

	void vulkanRenderer::loadExtensions()
	{
		vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT");
		if (!vkCmdDrawMeshTasksEXT)
			mErr = { "can't load extensions" };
	}

	void vulkanRenderer::init_pipelines()
	{
		init_background_pipelines();
		if (mErr)
			return;

		init_triangle_pipeline();
		if (mErr)
			return;

	}

	void vulkanRenderer::init_background_pipelines()
	{
		//layout code
		VkShaderModule computeDrawShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkCompiled/vkCompute.spv", _device, &computeDrawShader))
		{
			mErr = { "Error when building the compute shader" };
			return;
		}

		mComputePipeline.init(_device);
		mComputePipeline.setShader(computeDrawShader);
		mErr = mComputePipeline.build(VK_NULL_HANDLE, { mDescriptorSetCompute.getDescriptorSet().second });
		if (mErr)
			return;

		vkDestroyShaderModule(_device, computeDrawShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mComputePipeline.destroy(); });
	}

	void vulkanRenderer::init_triangle_pipeline()
	{
		VkShaderModule triangleFragShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkCompiled/vkMeshPs.spv", _device, &triangleFragShader))
		{
			mErr = { "Error when building meshlet shader." };
			return;
		}

		VkShaderModule meshShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkCompiled/vkMeshMs.spv", _device, &meshShader))
		{
			mErr = { "Error when building pixel shader." };
			return;
		}

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(pushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;

		mGraphicsPipeline.init(_device);
		//connecting the vertex and pixel shaders to the pipeline
		mGraphicsPipeline.setShaders(VK_NULL_HANDLE, meshShader, triangleFragShader);
		//it will draw triangles
		mGraphicsPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		mGraphicsPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		mGraphicsPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		//no multisampling
		mGraphicsPipeline.setMultisamplingNone();
		//no blending
		mGraphicsPipeline.disableBlending();
		mGraphicsPipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

		//connect the image format we will draw into, from draw image
		mGraphicsPipeline.setColorAttachmentFormat(mSwapChain.getDrawImageFormat());
		mGraphicsPipeline.setDepthFormat(mSwapChain.getDepthImageFormat());

		//finally build the pipeline.
		mErr = mGraphicsPipeline.build(&pushConstant, { mDescriptorSetMesh.getDescriptorSet().second }, true);
		if (mErr)
			return;

		//clean structures.
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, meshShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mGraphicsPipeline.destroy(); });
	}

	void vulkanRenderer::set_compute_descriptors()
	{
		std::vector<VkDescriptorImageInfo> imgInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = mSwapChain.getDrawImageView(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL, }
		};

		mDescriptorSetCompute.addBinding(
			descriptorSet::getLayoutBindingInfo(0, uint32_t(imgInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		);

		mDescriptorSetCompute.addWrite(descriptorSet::getWriteInfo(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imgInfo));

		mErr = mDescriptorSetCompute.build(VK_SHADER_STAGE_COMPUTE_BIT);
		if (mErr)
			return;
	}

	void vulkanRenderer::set_trinagle_descriptor_bindings()
	{
		std::vector<VkDescriptorBufferInfo> vertexInfo = {
			{.buffer = mVertex.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE },
		};
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(0, uint32_t(vertexInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		mDescriptorSetMesh.addWrite(descriptorSet::getWriteInfo(0, vertexInfo));

		std::vector<VkDescriptorBufferInfo> indexInfo = {
			{.buffer = mIndex.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE },
		};
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(1, uint32_t(indexInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		mDescriptorSetMesh.addWrite(descriptorSet::getWriteInfo(1, indexInfo));

		std::vector<VkDescriptorBufferInfo> primitiveInfo = {
			{.buffer = mTriangles.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE },
		};
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(2, uint32_t(primitiveInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		mDescriptorSetMesh.addWrite(descriptorSet::getWriteInfo(2, primitiveInfo));

		std::vector<VkDescriptorBufferInfo> meshletInfo = {
			{.buffer = mMeshlets.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE },
		};
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(3, uint32_t(meshletInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		mDescriptorSetMesh.addWrite(descriptorSet::getWriteInfo(3, meshletInfo));

		// !!!!!!!!!!!!!!!!!!!!!!! load albedo images.

		// get sampler.
		auto sampler = descriptorSet::createSampler(_device);
		if (!sampler)
		{
			mErr = sampler.err();
			return;
		}

		mImageSampler = sampler.value();

		_mainDeletionQueue.push_function([&]() { vkDestroySampler(_device, mImageSampler, nullptr); });

		// add write for each albedo image.
		std::vector<VkDescriptorImageInfo> imageInfos;
		for (auto& i : mAlbedoTextures)
		{
			VkDescriptorImageInfo info{};
			info.sampler = mImageSampler;
			info.imageView = i.image.view;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			imageInfos.push_back(info);
		}

		// add binding for desired number of albedo textures.
		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(4, uint32_t(imageInfos.size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		);
		mDescriptorSetMesh.addWrite(descriptorSet::getWriteInfo(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfos));

		mErr = mDescriptorSetMesh.build(VK_SHADER_STAGE_ALL);
		if (mErr)
			return;
	}

	void vulkanRenderer::initTextures()
	{
		initAlbedoTextures();
		if (mErr)
			return;
	}

	void vulkanRenderer::initAlbedoTextures()
	{
		//TODO: MOVE CALL BELOW TO ASSEETMANAGER.
#ifdef VULKAN
		stbi_set_flip_vertically_on_load(true);
#endif // VULKAN

		int width, height, nrChannels;
		uint8_t* data = stbi_load("../assets/textures/backpack_albedo.jpg", &width, &height, &nrChannels, 4);
		if (!data)
		{
			mErr = { "can't load texture" };
			return;
		}

		VkImageUsageFlags usage = 0;
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;       // Needed to copy/upload from a staging buffer
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;            // Needed to read in a shader
		usage |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;	// GPU only memmory.
		// Optional if you generate mipmaps on GPU:
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;       // To generate mipmaps

		vulkanImage crntImage;
		crntImage.init(_device, _allocator);
		mErr = crntImage.build(mImmediateSubmit, data, VkExtent3D{ .width = uint32_t(width), .height = uint32_t(height), .depth = 1 }, VK_FORMAT_R8G8B8A8_UNORM, usage, true);
		if (mErr)
			return;

		stbi_image_free(data);

		mAlbedoTextures.push_back(crntImage);

		_mainDeletionQueue.push_function([&]() {
			for (auto& i : mAlbedoTextures)
			{
				i.destroy();
			}
			});
	}
}

namespace vktest
{
	void vulkanRenderer::initMesh()
	{
		LOGINFO("loading mesh");

		std::string str = "../assets/backpack.glb";
		std::filesystem::path pathObj(str);

		std::vector<engine::vertex> v;
		std::vector<uint32_t> i;

		if (loadMeshFromGLTF(pathObj, v, i))
			LOGINFO("mesh loaded");

		LOGINFO("SUCCESS index len: {}, vertex len: {}", i.size(), v.size());

		LOGINFO("meshopt start");

		const size_t kMaxVertices = 64;
		const size_t kMaxTriangles = 64;
		const float  kConeWeight = 0.5f;

		std::vector<unsigned int> remap(i.size());
		size_t vertex_count = meshopt_generateVertexRemap(&remap[0], i.data(), i.size(),
			&v[0].position, v.size(), sizeof(engine::vertex));

		std::vector<engine::vertex> newVert(vertex_count);
		std::vector<uint32_t> newIndex(i.size());

		meshopt_remapIndexBuffer(newIndex.data(), i.data(), i.size(), &remap[0]);
		meshopt_remapVertexBuffer(newVert.data(), v.data(), v.size(), sizeof(engine::vertex), &remap[0]);

		const size_t maxMeshlets = meshopt_buildMeshletsBound(newIndex.size(), kMaxVertices, kMaxTriangles);

		std::vector<meshopt_Meshlet> meshlets;
		std::vector<uint32_t> meshletVertices;
		std::vector<uint8_t> meshletTriangles;

		meshlets.resize(maxMeshlets);
		meshletVertices.resize(maxMeshlets * kMaxVertices);
		meshletTriangles.resize(maxMeshlets * kMaxTriangles * 3);

		size_t meshletCount = meshopt_buildMeshlets(
			meshlets.data(),							// Output: array of meshopt_Meshlet
			meshletVertices.data(),						// Output: array of uint32_t - meshlet to mesh index mappings
			meshletTriangles.data(),					// Output: array of uint8_t - triangle indices
			newIndex.data(),							// Input: pointer mesh vertex indices
			newIndex.size(),							// Input: number of vertex indices
			&newVert[0].position.x,						// Input: pointer to vertex positions
			newVert.size(),								// Input: number of vertex positions	
			sizeof(engine::vertex),						// Input: stride of vertex position elements
			kMaxVertices,								// Input: maximum number of vertices per meshlet
			kMaxTriangles,								// Input: maximum number of triangles per meshlet
			kConeWeight									// Input: cone weight (we'll discuss this eventually...maybe)
		);

		auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
		meshlets.resize(meshletCount);

		std::vector<uint32_t> meshletTrianglesU32;
		for (auto& m : meshlets) {
			// Save triangle offset for current meshlet
			uint32_t triangleOffset = static_cast<uint32_t>(meshletTrianglesU32.size());

			// Repack to uint32_t
			for (uint32_t i = 0; i < m.triangle_count; ++i) {
				uint32_t i0 = 3 * i + 0 + m.triangle_offset;
				uint32_t i1 = 3 * i + 1 + m.triangle_offset;
				uint32_t i2 = 3 * i + 2 + m.triangle_offset;

				uint8_t  vIdx0 = meshletTriangles[i0];
				uint8_t  vIdx1 = meshletTriangles[i1];
				uint8_t  vIdx2 = meshletTriangles[i2];
				uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
					((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
					((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
				meshletTrianglesU32.push_back(packed);
			}

			// Update triangle offset for current meshlet
			m.triangle_offset = triangleOffset;
		}

		LOGINFO(
			"meshOpt end vertexCount: {}, triagnleCount: {}, meshletsCount: {}, indexBufferCount: {}",
			newVert.size(),
			meshletTriangles.size(),
			meshlets.size(),
			meshletVertices.size()
		);

		meshletsCount = meshlets.size();

		mVertex.init(_device, _allocator);
		mIndex.init(_device, _allocator);
		mTriangles.init(_device, _allocator);
		mMeshlets.init(_device, _allocator);

		auto err = mVertex.build(mImmediateSubmit, newVert.data(), newVert.size() * sizeof(engine::vertex), newVert.size() * sizeof(engine::vertex));
		if (err)
			LOGERROR(err.err());

		err = mIndex.build(mImmediateSubmit, meshletVertices.data(), meshletVertices.size() * sizeof(uint32_t), meshletVertices.size() * sizeof(uint32_t));
		if (err)
			LOGERROR(err.err());

		err = mMeshlets.build(mImmediateSubmit, meshlets.data(), meshlets.size() * sizeof(meshopt_Meshlet), meshlets.size() * sizeof(meshopt_Meshlet));
		if (err)
			LOGERROR(err.err());

		err = mTriangles.build(mImmediateSubmit, meshletTrianglesU32.data(), meshletTrianglesU32.size() * sizeof(uint32_t), meshletTrianglesU32.size() * sizeof(uint32_t));
		if (err)
			LOGERROR(err.err());

		_mainDeletionQueue.push_function([&] {
			mMeshlets.destroy();
			});

		_mainDeletionQueue.push_function([&] {
			mTriangles.destroy();
			});

		_mainDeletionQueue.push_function([&] {
			mIndex.destroy();
			});

		_mainDeletionQueue.push_function([&] {
			mVertex.destroy();
			});
	}

	vulkanRenderer::vulkanRenderer(std::shared_ptr<engine::context> ctx)
		:
		mCtx(ctx),
		mErr(),
		_instance(VK_NULL_HANDLE),
		_debug_messenger(VK_NULL_HANDLE),
		_chosenGPU(VK_NULL_HANDLE),
		_device(VK_NULL_HANDLE),
		_surface(VK_NULL_HANDLE),
		_graphicsQueue(),
		_graphicsQueueFamily(),
		mCamera(ctx, ctx->config.inner.camera.fov, ctx->config.inner.camera.nearPlane, ctx->config.inner.camera.farPlane, ctx->config.inner.wnd.width, ctx->config.inner.wnd.height),
		mGraphicsPipeline(),
		mDescriptorSetCompute(),
		mComputePipeline(),
		mImmediateSubmit(),
		mSwapChain()
	{
	};

	vulkanRenderer::~vulkanRenderer()
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		_mainDeletionQueue.flush();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
	}

	engine::error vulkanRenderer::checkError()
	{
		return mErr;
	}

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSetCompute.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, 0, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().width) / 16.0)), uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().height) / 16.0)), 1);
	}

	void vulkanRenderer::draw()
	{
		auto waitResult = mSwapChain.waitOnCurrentFence();
		if (waitResult)
		{
			mErr = waitResult.err();
			return;
		}

		mSwapChain.pickImageExtent();

		// request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		auto indexResult = mSwapChain.acquireImageIndex();
		if (!indexResult)
		{
			mErr = indexResult.err();
			return;
		}

		auto resetRes = mSwapChain.resetCommandBuffer();
		if (resetRes)
		{
			mErr = resetRes.err();
			return;
		}

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = mSwapChain.getCurrentFrameData().commandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// start recording.
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		vkinit::transition_image(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		vkinit::transition_image(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkinit::transition_image(cmd, mSwapChain.getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		draw_geometry(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		vkinit::transition_image(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the draw image into the swapchain
		vkinit::copy_image_to_image(cmd, mSwapChain.getDrawImage(), mSwapChain.getSwapChainImages()[indexResult.value()], mSwapChain.getDrawImageExtent(), mSwapChain.getSwapChainExtent());

		// set swapchain image layout to Attachment Optimal so we can draw it
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));

		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// (remember when we ask GPU for image from swap chain we provide that semaphore to signal.)
		// we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getCurrentFrameData().swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getCurrentFrameData().renderSemaphore);

		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, mSwapChain.getCurrentFrameData().renderFence));

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		mSwapChain.present(_graphicsQueue, indexResult.value());
	}

	void vulkanRenderer::resize(uint32_t width, uint32_t height)
	{
		mSwapChain.destroy();
		mSwapChain.build(width, height, _graphicsQueueFamily);

		// reconfigure source for destroyed imageView.
		mDescriptorSetCompute.clearBindings();
		mDescriptorSetCompute.destroy();

		set_compute_descriptors();

		mCamera.changeViewPort(width, height);
	}

	void vulkanRenderer::draw_geometry(VkCommandBuffer cmd)
	{
		//begin a render pass connected to our draw image.
		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(mSwapChain.getDrawImageView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(mSwapChain.getDepthImageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkinit::rendering_info(mSwapChain.getDrawImageExtent(), &colorAttachment, &depthAttachment);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline.getPipeline().first);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = float(mSwapChain.getDrawImageExtent().width);
		viewport.height = float(mSwapChain.getDrawImageExtent().height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (mSwapChain.getDrawImageExtent().width);
		scissor.extent.height = (mSwapChain.getDrawImageExtent().height);

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// set push constants.
		pushConstants pc;
		pc.viewProjection = mCamera.getProjection() * mCamera.getCameraTransform();

		vkCmdPushConstants(cmd, mGraphicsPipeline.getPipeline().second, VK_SHADER_STAGE_MESH_BIT_EXT, 0, sizeof(pushConstants), &pc);

		// bind the descriptor set.
		auto set = mDescriptorSetMesh.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline.getPipeline().second, 0, 1, &set, 0, nullptr);

		//launch a draw command to draw 3 vertices
		//vkCmdDraw(cmd, 67164, 1, 0, 0);
		vkCmdDrawMeshTasksEXT(cmd, uint32_t(meshletsCount), 1, 1);

		vkCmdEndRendering(cmd);
	}

	void vulkanRenderer::changeCameraPos(glm::vec3 shift)
	{
		mCamera.changePosition(shift);
	}

	void vulkanRenderer::changeYaw(float yaw)
	{
		mCamera.changeYaw(yaw);
	}

	void vulkanRenderer::changePitch(float pitch)
	{
		mCamera.changePitch(pitch);
	}

	void vulkanRenderer::test()
	{
	}
}

