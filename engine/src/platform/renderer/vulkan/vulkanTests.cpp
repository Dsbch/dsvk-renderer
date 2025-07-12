#include <pch.h>

#include "vulkanTests.h"
#include "platform/window/win32/window.h"
#include "vkHelper.h"

#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

//< node_types
//> intro
#define VK_CHECK(x)                                                 \
    {\
		VkResult err = x;                                               \
		if (err != VK_SUCCESS)                                          \
			LOGERROR(vkResultToStr(err));                               \
	}\
//< intro

// vulkan part.
namespace vktest
{
	struct DescriptorAllocator {

		struct PoolSizeRatio {
			VkDescriptorType type;
			float ratio;
		};

		VkDescriptorPool pool;

		void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
		void clear_descriptors(VkDevice device);
		void destroy_pool(VkDevice device);

		VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
	};

	struct DescriptorLayoutBuilder {

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		void add_binding(uint32_t binding, VkDescriptorType type);
		void clear();
		VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
	{
		VkDescriptorSetLayoutBinding newbind{};
		newbind.binding = binding;
		newbind.descriptorCount = 1;
		newbind.descriptorType = type;

		bindings.push_back(newbind);
	}

	void DescriptorLayoutBuilder::clear()
	{
		bindings.clear();
	}

	VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& b : bindings) {
			b.stageFlags |= shaderStages;
		}

		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.pNext = pNext;

		info.pBindings = bindings.data();
		info.bindingCount = (uint32_t)bindings.size();
		info.flags = flags;

		VkDescriptorSetLayout set;
		VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

		return set;
	}

	struct AllocatedImage {
		VkImage image;
		VkImageView imageView;
		VmaAllocation allocation;
		VkExtent3D imageExtent;
		VkFormat imageFormat;
	};

	// to do rewrite from using std::function to concrete vulkan handles may be using std::variant?
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void flush()
		{
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
			{
				(*it)();
			}

			deletors.clear();
		}
	};

	struct FrameData {

		VkCommandPool _commandPool;
		VkCommandBuffer _mainCommandBuffer;

		VkSemaphore _swapchainSemaphore, _renderSemaphore;
		VkFence _renderFence;

		DeletionQueue _deletionQueue;
	};

	constexpr unsigned int FRAME_OVERLAP = 2;

	class vulkanRenderer
	{
	public:
		vulkanRenderer(std::shared_ptr<engine::context> ctx)
			:
			mCtx(ctx),
			mErr(),
			_instance(VK_NULL_HANDLE),
			_debug_messenger(VK_NULL_HANDLE),
			_chosenGPU(VK_NULL_HANDLE),
			_device(VK_NULL_HANDLE),
			_surface(VK_NULL_HANDLE),
			_frameNumber(0),
			_swapchain(),
			_swapchainImageFormat(),
			_swapchainExtent(),
			_graphicsQueue(),
			_graphicsQueueFamily()
		{};

		~vulkanRenderer();
		engine::error checkError();

		void init(engine::winApiWindow* window);

		void draw_background(VkCommandBuffer cmd);
		void draw();

		VkSwapchainKHR _swapchain;
		VkFormat _swapchainImageFormat;

		std::vector<VkImage> _swapchainImages;
		std::vector<VkImageView> _swapchainImageViews;
		VkExtent2D _swapchainExtent;

		FrameData _frames[FRAME_OVERLAP];

		uint32_t _frameNumber;

		FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; };

		VkQueue _graphicsQueue;
		uint32_t _graphicsQueueFamily;

		DescriptorAllocator globalDescriptorAllocator;

		VkDescriptorSet _drawImageDescriptors;
		VkDescriptorSetLayout _drawImageDescriptorLayout;

		VkPipeline _gradientPipeline;
		VkPipelineLayout _gradientPipelineLayout;

		void resize_swapchain(uint32_t width, uint32_t height);
	private:
		engine::error mErr;
		std::shared_ptr<engine::context> mCtx;

		VkInstance _instance;// Vulkan library handle
		VkDebugUtilsMessengerEXT _debug_messenger;// Vulkan debug output handle
		VkPhysicalDevice _chosenGPU;// GPU chosen as the default device
		VkDevice _device; // Vulkan device for commands
		VkSurfaceKHR _surface;// Vulkan window surface
		DeletionQueue _mainDeletionQueue;

		VmaAllocator _allocator;

		AllocatedImage _drawImage;
		VkExtent2D _drawExtent;

		void printGPU()
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

			auto deviceTypeToString = [](VkPhysicalDeviceType type) -> const char* {
				switch (type) {
				case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "Other";
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete GPU";
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual GPU";
				case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
				default:                                     return "Unknown";
				}
				};

			for (uint32_t i = 0; i < deviceCount; ++i)
			{
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(devices[i], &props);

				LOGINFO("GPU {}: {} (type = {}, api version = {}.{}.{})", i,
					props.deviceName,
					deviceTypeToString(props.deviceType),
					VK_VERSION_MAJOR(props.apiVersion),
					VK_VERSION_MINOR(props.apiVersion),
					VK_VERSION_PATCH(props.apiVersion));
			}
		}


		void init_swapchain(uint32_t width, uint32_t height);
		void destroy_swapchain();

		void init_vulkan(engine::winApiWindow* window);
		void init_commands();
		void init_sync_structures();

		void create_swapchain(uint32_t width, uint32_t height);

		void init_descriptors();

		void init_pipelines();
		void init_background_pipelines();
	};


	vulkanRenderer::~vulkanRenderer()
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
		}

		_mainDeletionQueue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
	}

	engine::error vulkanRenderer::checkError()
	{
		return mErr;
	}

	void vulkanRenderer::init(engine::winApiWindow* window)
	{
		init_vulkan(window);

		init_swapchain(window->getWidth(), window->getHeight());

		init_commands();

		init_sync_structures();

		init_descriptors();

		init_pipelines();
	}

	void vulkanRenderer::draw_background(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

		// bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(_drawExtent.width) / 16.0)), uint32_t(std::ceil(double(_drawExtent.height) / 16.0)), 1);
	}

	void vulkanRenderer::draw()
	{
		_drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height);
		_drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width);

		//wait until the gpu has finished rendering the last frame. Timeout of 1 second
		VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

		get_current_frame()._deletionQueue.flush();

		//request image from the swapchain
		uint32_t swapchainImageIndex;

		VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}

		VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

		//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		//> draw_first
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		vkinit::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		draw_background(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		vkinit::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//< draw_first
		//> imgui_draw
			// execute a copy from the draw image into the swapchain
		vkinit::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

		// set swapchain image layout to Attachment Optimal so we can draw it
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));
		//< imgui_draw

			//prepare the submission to the queue. 
			//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
			//we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		//submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = vktest::present_info();

		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		vkQueuePresentKHR(_graphicsQueue, &presentInfo);


		//increase the number of frames drawn
		_frameNumber++;
	}

	void vulkanRenderer::resize_swapchain(uint32_t width, uint32_t height)
	{
		vkDeviceWaitIdle(_device);

		destroy_swapchain();

		create_swapchain(width, height);
	}

	void vulkanRenderer::init_vulkan(engine::winApiWindow* window)
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

		vkb::Instance vkb_inst = inst_ret.value();

		//grab the instance 
		_instance = vkb_inst.instance;
		_debug_messenger = vkb_inst.debug_messenger;

		//< init_instance
		// 
		//> init_device
		// create surface.
		VkWin32SurfaceCreateInfoKHR surfaceInfo{};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = window->getHandle();
		surfaceInfo.hinstance = window->getInstance();

		VkResult result = vkCreateWin32SurfaceKHR(_instance, &surfaceInfo, nullptr, &_surface);
		if (result != VK_SUCCESS) {
			mErr = { "Failed to create Win32 surface" };
			return;
		}

		// Print all gpus.
		printGPU();

		//vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features.dynamicRendering = true;
		features.synchronization2 = true;

		//vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;


		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(_surface)
			.select()
			.value();


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

		_mainDeletionQueue.push_function([&]() {
			vmaDestroyAllocator(_allocator);
			});
	}

	void vulkanRenderer::init_swapchain(uint32_t width, uint32_t height)
	{
		create_swapchain(width, height);

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

		//add to deletion queues
		_mainDeletionQueue.push_function([=]() {
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
			});
	}

	void vulkanRenderer::init_commands()
	{
		//create a command pool for commands submitted to the graphics queue.
		//we also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

			// allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
		}
	}

	void vulkanRenderer::init_sync_structures()
	{
		//create syncronization structures
		//one fence to control when the gpu has finished rendering the frame,
		//and 2 semaphores to syncronize rendering with swapchain
		//we want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
		}
	}

	void vulkanRenderer::create_swapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

		_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		_swapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	void vulkanRenderer::destroy_swapchain()
	{
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < _swapchainImageViews.size(); i++) {

			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
	}

	void vulkanRenderer::init_descriptors()
	{
		//create a descriptor pool that will hold 10 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
		};

		globalDescriptorAllocator.init_pool(_device, 10, sizes);

		//make the descriptor set layout for our compute draw
		{
			DescriptorLayoutBuilder builder;
			builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			_drawImageDescriptorLayout = builder.build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		_drawImageDescriptors = globalDescriptorAllocator.allocate(_device, _drawImageDescriptorLayout);

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = _drawImage.imageView;

		VkWriteDescriptorSet drawImageWrite = {};
		drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		drawImageWrite.pNext = nullptr;

		drawImageWrite.dstBinding = 0;
		drawImageWrite.dstSet = _drawImageDescriptors;
		drawImageWrite.descriptorCount = 1;
		drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		drawImageWrite.pImageInfo = &imgInfo;

		vkUpdateDescriptorSets(_device, 1, &drawImageWrite, 0, nullptr);

		//make sure both the descriptor allocator and the new layout get cleaned up properly
		_mainDeletionQueue.push_function([&]() {
			globalDescriptorAllocator.destroy_pool(_device);

			vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
			});
	}

	void vulkanRenderer::init_pipelines()
	{
		init_background_pipelines();
	}

	void vulkanRenderer::init_background_pipelines()
	{
		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
		computeLayout.setLayoutCount = 1;

		VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

		//layout code
		VkShaderModule computeDrawShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkCompute.glsl.spv", _device, &computeDrawShader))
		{
			fmt::print("Error when building the compute shader \n");
		}

		VkPipelineShaderStageCreateInfo stageinfo{};
		stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageinfo.pNext = nullptr;
		stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageinfo.module = computeDrawShader;
		stageinfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = _gradientPipelineLayout;
		computePipelineCreateInfo.stage = stageinfo;

		VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));

		vkDestroyShaderModule(_device, computeDrawShader, nullptr);

		_mainDeletionQueue.push_function([&]() {
			vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
			vkDestroyPipeline(_device, _gradientPipeline, nullptr);
			});
	}

	void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio ratio : poolRatios) {
			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.type,
				.descriptorCount = uint32_t(ratio.ratio * maxSets)
				});
		}

		VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		pool_info.flags = 0;
		pool_info.maxSets = maxSets;
		pool_info.poolSizeCount = (uint32_t)poolSizes.size();
		pool_info.pPoolSizes = poolSizes.data();

		vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
	}

	void DescriptorAllocator::clear_descriptors(VkDevice device)
	{
		vkResetDescriptorPool(device, pool, 0);

	}

	void DescriptorAllocator::destroy_pool(VkDevice device)
	{
		vkDestroyDescriptorPool(device, pool, nullptr);
	}

	VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet ds;
		VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

		return ds;
	}
}

// app part.
namespace vktest
{
	vulkanTest::vulkanTest(std::shared_ptr<engine::context> ctx) :
		mCtx(ctx),
		mRenderer(std::make_unique<vulkanRenderer>(ctx)),
		mWindow(nullptr)
	{
		engine::cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = engine::windowFactory::createWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] { return mWindow.get(); });

		mRenderer->init(static_cast<engine::winApiWindow*>(mWindow.get()));
		if (mRenderer->checkError())
			return;
	}

	engine::error vulkanTest::checkError()
	{
		return mErr;
	}

	vulkanTest::~vulkanTest()
	{
	}

	void vulkanTest::run()
	{
		while (true)
		{
			// process events.
			while (mCtx->mEventDispatcher->hasEvents())
			{
				auto event = mCtx->mEventDispatcher->getEvent();

				if (event->getEventType() == engine::close)
				{
					LOGINFO("app was closed");
					return;
				}

				if (event->getEventType() == engine::windowResize)
				{
					auto resizeEvent = static_cast<engine::windowResizeEvent*>(event.get());

					mRenderer->resize_swapchain(resizeEvent->getWidth(), resizeEvent->getHeight());
				}
			}

			// do rendering here.
			mRenderer->draw();
		}
	}

}