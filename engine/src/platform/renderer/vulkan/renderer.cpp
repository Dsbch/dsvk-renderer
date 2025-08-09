#include <pch.h>

#define VMA_IMPLEMENTATION
#include "renderer.h"

#include "platform/renderer/vertex.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <meshoptimizer.h>

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

	cgltf_options options = {};
	cgltf_data* data = nullptr;

	if (cgltf_parse(&options, fileData.data(), fileData.size(), &data) != cgltf_result_success)
		return false;

	if (cgltf_load_buffers(&options, data, path.parent_path().string().c_str()) != cgltf_result_success) {
		cgltf_free(data);
		return false;
	}

	outVertices.clear();
	outIndices.clear();

	for (size_t mi = 0; mi < data->meshes_count; ++mi) {
		const cgltf_mesh& mesh = data->meshes[mi];

		for (size_t pri = 0; pri < mesh.primitives_count; ++pri) {
			const cgltf_primitive& prim = mesh.primitives[pri];
			if (prim.type != cgltf_primitive_type_triangles)
				continue;

			const cgltf_accessor* positionAccessor = nullptr;
			const cgltf_accessor* normalAccessor = nullptr;
			const cgltf_accessor* texcoordAccessor = nullptr;

			for (size_t ai = 0; ai < prim.attributes_count; ++ai) {
				const cgltf_attribute& attr = prim.attributes[ai];
				if (strcmp(attr.name, "POSITION") == 0) positionAccessor = attr.data;
				else if (strcmp(attr.name, "NORMAL") == 0) normalAccessor = attr.data;
				else if (strcmp(attr.name, "TEXCOORD_0") == 0) texcoordAccessor = attr.data;
			}

			if (!positionAccessor || positionAccessor->component_type != cgltf_component_type_r_32f || positionAccessor->type != cgltf_type_vec3)
				continue;

			size_t vertexCount = positionAccessor->count;
			uint32_t baseIndex = static_cast<uint32_t>(outVertices.size());

			for (size_t i = 0; i < vertexCount; ++i) {
				engine::vertex v = {};

				float pos[3] = {};
				cgltf_accessor_read_float(positionAccessor, i, pos, 3);
				v.position = glm::vec3(pos[0], pos[1], pos[2]);

				if (normalAccessor) {
					float norm[3] = {};
					cgltf_accessor_read_float(normalAccessor, i, norm, 3);
					v.normal = glm::vec3(norm[0], norm[1], norm[2]);
				}

				if (texcoordAccessor) {
					float uv[2] = {};
					cgltf_accessor_read_float(texcoordAccessor, i, uv, 2);
					v.textureCoords = glm::vec2(uv[0], uv[1]);
				}

				outVertices.push_back(v);
			}

			// Indices
			if (prim.indices) {
				const cgltf_accessor* indexAccessor = prim.indices;
				const uint8_t* buffer = reinterpret_cast<const uint8_t*>(
					indexAccessor->buffer_view->buffer->data) +
					indexAccessor->buffer_view->offset + indexAccessor->offset;

				for (size_t i = 0; i < indexAccessor->count; ++i) {
					uint32_t index = 0;
					switch (indexAccessor->component_type) {
					case cgltf_component_type_r_16u:
						index = reinterpret_cast<const uint16_t*>(buffer)[i]; break;
					case cgltf_component_type_r_32u:
						index = reinterpret_cast<const uint32_t*>(buffer)[i]; break;
					case cgltf_component_type_r_8u:
						index = reinterpret_cast<const uint8_t*>(buffer)[i]; break;
					default: continue;
					}
					outIndices.push_back(baseIndex + index);
				}
			}
		}
	}

	cgltf_free(data);
	return true;
}

struct pushConstants
{
	glm::mat4 viewProjection;
};

namespace vktest
{
	PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;

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

	void vulkanRenderer::init(engine::window* window)
	{
		init_vulkan(window);

		init_immediate_submit();

		initMesh();

		init_swapchain(window->getWidth(), window->getHeight());

		init_descriptors();

		init_pipelines();
	}

	void vulkanRenderer::init_pipelines()
	{
		init_background_pipelines();
		init_triangle_pipeline();
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

	void vulkanRenderer::init_background_pipelines()
	{
		//layout code
		VkShaderModule computeDrawShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkSimple.comp.spv", _device, &computeDrawShader))
		{
			fmt::print("Error when building the compute shader \n");
		}

		mComputePipeline.init(_device);
		mComputePipeline.setShader(computeDrawShader);
		mComputePipeline.build(VK_NULL_HANDLE, { mDescriptorSetCompute.getDescriptorSet().second });

		vkDestroyShaderModule(_device, computeDrawShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mComputePipeline.destroy(); });
	}

	void vulkanRenderer::init_triangle_pipeline()
	{
		VkShaderModule triangleFragShader;
		if (!vkinit::load_shader_module("../assets/shaders/meshlet_ps.spv", _device, &triangleFragShader)) {
			LOGERROR("Error when building the triangle fragment shader module");
		}
		else {
			LOGINFO("Triangle fragment shader succesfully loaded");
		}

		VkShaderModule meshShader;
		if (!vkinit::load_shader_module("../assets/shaders/meshlet_ms.spv", _device, &meshShader)) {
			LOGERROR("Error when building the triangle vertex shader module");
		}
		else {
			LOGINFO("Triangle vertex shader succesfully loaded");
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
		mGraphicsPipeline.setDepthFormat(mSwapChain.getDepthImageFormt());

		//finally build the pipeline.
		mGraphicsPipeline.build(&pushConstant, { mDescriptorSetMesh.getDescriptorSet().second }, true);

		//clean structures.
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, meshShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mGraphicsPipeline.destroy(); });
	}

	void vulkanRenderer::loadExtensions()
	{
		vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(_device, "vkCmdDrawMeshTasksEXT");
		if (!vkCmdDrawMeshTasksEXT)
			LOGERROR("can't load vkCmdDrawMeshTasksEXT");
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
		features12.pNext = &features13;

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto selectedRes = selector
			.set_minimum_version(1, 3)
			.set_required_features_12(features12)
			.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			//.add_required_extension(VK_NV_MESH_SHADER_EXTENSION_NAME)
			.add_required_extension(VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME)
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
		vmaCreateAllocator(&allocatorInfo, &_allocator);

		loadExtensions();

		_mainDeletionQueue.push_function([&]() {
			vmaDestroyAllocator(_allocator);
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
		mDescriptorSetCompute.init(_device);
		mDescriptorSetMesh.init(_device);

		set_trinagle_descriptor_bindings();
		set_compute_descriptors();

		_mainDeletionQueue.push_function([&]() { mDescriptorSetCompute.destroy(); });
		_mainDeletionQueue.push_function([&]() { mDescriptorSetMesh.destroy(); });
		_mainDeletionQueue.push_function([&]() { descriptorSet::destroyPool(); });
	}

	void vulkanRenderer::set_compute_descriptors()
	{
		VkDescriptorSetLayoutBinding imageBind{};
		imageBind.binding = 0;
		imageBind.descriptorCount = 1;
		imageBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = mSwapChain.getDrawImage().imageView;

		VkWriteDescriptorSet source = {};
		source.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		source.pNext = nullptr;
		source.dstBinding = 0;
		// will be set by descriptorSet class.
		source.dstSet = nullptr;
		source.descriptorCount = 1;
		source.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		source.pImageInfo = &imgInfo;

		mDescriptorSetCompute.addBinding(imageBind, source);
		mErr = mDescriptorSetCompute.build(VK_SHADER_STAGE_COMPUTE_BIT);
		if (mErr)
			LOGERROR(mErr.err());
	}

	void vulkanRenderer::set_trinagle_descriptor_bindings()
	{
		VkDescriptorSetLayoutBinding bufferBinds[4] = {};
		for (uint32_t i = 0; i < 4; ++i)
		{
			bufferBinds[i].binding = i;
			bufferBinds[i].descriptorCount = 1;
			bufferBinds[i].stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
			bufferBinds[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		}

		VkDescriptorBufferInfo bufferInfos[4] = {};
		VkWriteDescriptorSet sourceBuffer[4] = {};

		bufferInfos[0].buffer = mVertex.getBuffer().buffer;
		bufferInfos[0].offset = 0;
		bufferInfos[0].range = VK_WHOLE_SIZE;

		sourceBuffer[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sourceBuffer[0].pNext = nullptr;
		sourceBuffer[0].dstSet = VK_NULL_HANDLE;        // будет установлен позже
		sourceBuffer[0].dstBinding = 0;
		sourceBuffer[0].dstArrayElement = 0;
		sourceBuffer[0].descriptorCount = 1;
		sourceBuffer[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sourceBuffer[0].pImageInfo = nullptr;
		sourceBuffer[0].pBufferInfo = &bufferInfos[0];
		sourceBuffer[0].pTexelBufferView = nullptr;

		bufferInfos[1].buffer = mIndex.getBuffer().buffer;
		bufferInfos[1].offset = 0;
		bufferInfos[1].range = VK_WHOLE_SIZE;

		sourceBuffer[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sourceBuffer[1].pNext = nullptr;
		sourceBuffer[1].dstSet = VK_NULL_HANDLE;
		sourceBuffer[1].dstBinding = 1;
		sourceBuffer[1].dstArrayElement = 0;
		sourceBuffer[1].descriptorCount = 1;
		sourceBuffer[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sourceBuffer[1].pImageInfo = nullptr;
		sourceBuffer[1].pBufferInfo = &bufferInfos[1];
		sourceBuffer[1].pTexelBufferView = nullptr;

		bufferInfos[2].buffer = mTriangles.getBuffer().buffer;
		bufferInfos[2].offset = 0;
		bufferInfos[2].range = VK_WHOLE_SIZE;

		sourceBuffer[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sourceBuffer[2].pNext = nullptr;
		sourceBuffer[2].dstSet = VK_NULL_HANDLE;
		sourceBuffer[2].dstBinding = 2;
		sourceBuffer[2].dstArrayElement = 0;
		sourceBuffer[2].descriptorCount = 1;
		sourceBuffer[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sourceBuffer[2].pImageInfo = nullptr;
		sourceBuffer[2].pBufferInfo = &bufferInfos[2];
		sourceBuffer[2].pTexelBufferView = nullptr;

		bufferInfos[3].buffer = mMeshlets.getBuffer().buffer;
		bufferInfos[3].offset = 0;
		bufferInfos[3].range = VK_WHOLE_SIZE;

		sourceBuffer[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		sourceBuffer[3].pNext = nullptr;
		sourceBuffer[3].dstSet = VK_NULL_HANDLE;
		sourceBuffer[3].dstBinding = 3;
		sourceBuffer[3].dstArrayElement = 0;
		sourceBuffer[3].descriptorCount = 1;
		sourceBuffer[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		sourceBuffer[3].pImageInfo = nullptr;
		sourceBuffer[3].pBufferInfo = &bufferInfos[3];
		sourceBuffer[3].pTexelBufferView = nullptr;

		for (uint32_t i = 0; i < 4; ++i)
		{
			mDescriptorSetMesh.addBinding(bufferBinds[i], sourceBuffer[i]);
		}

		mErr = mDescriptorSetMesh.build(VK_SHADER_STAGE_MESH_BIT_EXT);
		if (mErr)
			LOGERROR(mErr.err());
	}

	void vulkanRenderer::initMesh()
	{
		LOGINFO("loading mesh");

		std::string str = "../assets/horse_statue_01_4k.glb";
		//std::string str = "../assets/cube.glb";
		std::filesystem::path pathObj(str);

		std::vector<engine::vertex> v;
		std::vector<uint32_t> i;

		if (loadMeshFromGLTF(pathObj, v, i))
			LOGINFO("mesh loaded");

		LOGINFO("SUCCESS index len: {}, vertex len: {}", i.size(), v.size());

		LOGINFO("meshopt start");

		const size_t kMaxVertices = 64;
		const size_t kMaxTriangles = 124;
		const float  kConeWeight = 0.0f;

		const size_t maxMeshlets = meshopt_buildMeshletsBound(i.size(), kMaxVertices, kMaxTriangles);

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
			i.data(),									// Input: pointer mesh vertex indices
			i.size(),									// Input: number of vertex indices
			reinterpret_cast<const float*>(v.data()),	// Input: pointer to vertex positions
			v.size(),									// Input: number of vertex positions	
			sizeof(engine::vertex),						// Input: stride of vertex position elements
			kMaxVertices,								// Input: maximum number of vertices per meshlet
			kMaxTriangles,								// Input: maximum number of triangles per meshlet
			kConeWeight									// Input: cone weight (we'll discuss this eventually...maybe)
		);

		auto& last = meshlets[meshletCount - 1];
		meshletVertices.resize(last.vertex_offset + last.vertex_count);
		meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
		meshlets.resize(meshletCount);

		LOGINFO(
			"meshOpt end vertexCount: {}, triagnleCount: {}, meshletsCount: {}, indexBufferCount: {}",
			v.size(),
			meshletTriangles.size(),
			meshlets.size(),
			meshletVertices.size()
		);

		std::vector<uint32_t> meshletTriangles32;
		for (auto k : meshletTriangles)
			meshletTriangles32.push_back(uint32_t(k));

		mVertex.init(_device, _allocator);
		mIndex.init(_device, _allocator);
		mTriangles.init(_device, _allocator);
		mMeshlets.init(_device, _allocator);

		auto err = mVertex.build(mImmediateSubmit, v.data(), v.size() * sizeof(engine::vertex), v.size());
		if (err)
			LOGERROR(err.err());

		err = mIndex.build(mImmediateSubmit, meshletVertices.data(), meshletVertices.size() * sizeof(uint32_t), meshletVertices.size());
		if (err)
			LOGERROR(err.err());

		err = mMeshlets.build(mImmediateSubmit, meshlets.data(), meshlets.size() * sizeof(meshopt_Meshlet), meshlets.size());
		if (err)
			LOGERROR(err.err());

		err = mTriangles.build(mImmediateSubmit, meshletTriangles32.data(), meshletTriangles32.size() * sizeof(uint32_t), meshletTriangles32.size());
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

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSetCompute.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, 0, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(mSwapChain.getDrawImage().imageExtent.width) / 16.0)), uint32_t(std::ceil(double(mSwapChain.getDrawImage().imageExtent.height) / 16.0)), 1);
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
		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkinit::transition_image(cmd, mSwapChain.getDepthImage().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		draw_geometry(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the draw image into the swapchain
		vkinit::copy_image_to_image(cmd, mSwapChain.getDrawImage().image, mSwapChain.getSwapChainImages()[indexResult.value()], mSwapChain.getDrawImage().imageExtent, mSwapChain.getSwapChainExtent());

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
		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(mSwapChain.getDrawImage().imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(mSwapChain.getDepthImage().imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkinit::rendering_info(mSwapChain.getDrawImage().imageExtent, &colorAttachment, &depthAttachment);

		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline.getPipeline().first);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = float(mSwapChain.getDrawImage().imageExtent.width);
		viewport.height = float(mSwapChain.getDrawImage().imageExtent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (mSwapChain.getDrawImage().imageExtent.width);
		scissor.extent.height = (mSwapChain.getDrawImage().imageExtent.height);

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
		vkCmdDrawMeshTasksEXT(cmd, 244, 1, 1);

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
}