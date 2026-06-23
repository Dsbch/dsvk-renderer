#include <pch.h>
#include "uiRenderer.h"
#include "helper.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace engine
{
	error uiRenderer::init(
		GLFWwindow* wnd,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkInstance instance,
		uint32_t queueFamily,
		VkQueue queue,
		swapChain sChain,
		graphicsPreset preset
	)
	{
		mDevice = device;
		mPreset = preset;

		auto samp = descriptorSet::createSampler(mDevice, float(mPreset.anisotropicFiltering));
		if (!samp)
			return samp.err();

		mSampler = samp.value();

		auto logResult = [](VkResult err)
			{
				if (err != VK_SUCCESS)
					LOGERROR(vkResultToStr(err));
			};

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		auto colorAttachmentFormat = sChain.getDrawImageFormat();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(wnd, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.QueueFamily = queueFamily;
		initInfo.Queue = queue;
		initInfo.DescriptorPoolSize = 100;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = 3;
		initInfo.PipelineInfoMain.Subpass = 0;
		initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &colorAttachmentFormat
		};
		initInfo.CheckVkResultFn = logResult;
		initInfo.UseDynamicRendering = true;

		bool result = ImGui_ImplVulkan_Init(&initInfo);
		if (!result)
			return error{ "can't init UI" };

		updateSwapchainDependentDescriptors(sChain);

		return error();
	}

	error uiRenderer::destroy()
	{
		for (auto& ds : mImGuiDescroptorSets)
			ImGui_ImplVulkan_RemoveTexture(ds);

		vkDestroySampler(mDevice, mSampler, nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		return {};
	}

	error uiRenderer::onRender(VkCommandBuffer cmd, profilingInfo profInfo)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Add calls to imgui here.
		renderProfilingInfo(profInfo);

		renderAccumAndRevealImages();

		renderHzbImages();

		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		return {};
	}

	void uiRenderer::updateSwapchainDependentDescriptors(swapChain sChain)
	{
		for (auto& ds : mImGuiDescroptorSets)
			ImGui_ImplVulkan_RemoveTexture(ds);

		mImGuiDescroptorSets.clear();

		VkDescriptorSet depthDescriptorSet = ImGui_ImplVulkan_AddTexture(
			mSampler,
			sChain.getAccumImageView(mPreset.msaa > 1),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		mImGuiDescroptorSets.push_back(depthDescriptorSet);

		auto hzb = sChain.getHZB();

		depthDescriptorSet = ImGui_ImplVulkan_AddTexture(
			mSampler,
			//hzb.front().img.view,
			sChain.getRevealImageView(mPreset.msaa > 1),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		mImGuiDescroptorSets.push_back(depthDescriptorSet);

		depthDescriptorSet = ImGui_ImplVulkan_AddTexture(
			mSampler,
			sChain.getDepthImageView(mPreset.msaa > 1),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		mImGuiDescroptorSets.push_back(depthDescriptorSet);

		std::vector<vulkanImage> hzbBuf = sChain.getHZB();

		for (auto& h : hzbBuf)
		{
			depthDescriptorSet = ImGui_ImplVulkan_AddTexture(
				mSampler,
				h.img.view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			mImGuiDescroptorSets.push_back(depthDescriptorSet);
		}
	}

	void uiRenderer::renderProfilingInfo(profilingInfo profInfo)
	{
		// Style
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

		char title[64];
		snprintf(title, sizeof(title), "Profiler [%.2f fps]###profiler", profInfo.globalInfo.fps);

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiCond_Once);
		
		ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoScrollbar);

		// 1.FPS.
		{
			static float fpsHistory[128] = {};
			static int offset = 0;
			static auto lastUpdate = std::chrono::steady_clock::now();

			auto now = std::chrono::steady_clock::now();
			if (std::chrono::duration<float, std::milli>(now - lastUpdate).count() >= 60.0f)
			{
				fpsHistory[offset] = profInfo.globalInfo.fps;
				offset = (offset + 1) % 128;
				lastUpdate = now;
			}

			char overlay[128];
			snprintf(overlay, sizeof(overlay), "%.2f fps | %.2f ms | dt: %.4f ms",
				profInfo.globalInfo.fps, 1000.0f / profInfo.globalInfo.fps, profInfo.globalInfo.deltaTime);

			ImGui::PlotLines("##fps", fpsHistory, 128, offset, overlay, 0.0f, 200.0f, ImVec2(0, 80));
			ImGui::Separator();
		}

		// 2. Render passes.
		{
			// 2.1 Update history.
			for (auto const& [name, ms] : profInfo.passInfo) 
			{
				if (mPassHistories.find(name) == mPassHistories.end())
					mPassHistories[name] = { {}, 0, 0.0f, IM_COL32(rand() % 200 + 55, rand() % 200 + 55, rand() % 200 + 55, 255) };

				auto& h = mPassHistories[name];
				h.history[h.offset] = ms;
				h.lastValidMs = ms;
				h.offset = (h.offset + 1) % 128;
			}

			// Calculate dynamic max for scaling.
			float maxMs = 1.0f;
			for (int f = 0; f < 128; f++)
			{
				float total = 0.0f;
				for (auto const& [name, h] : mPassHistories) total += h.history[f];
				maxMs = std::max(maxMs, total);
			}
			static float smoothMax = 1.0f;
			smoothMax += (maxMs - smoothMax) * 0.05f;

			// 2.2 Rendering.
			float graphHeight = 100.0f; 
			ImGui::BeginChild("ProfilerGraphArea", ImVec2(0, graphHeight), false, ImGuiWindowFlags_NoScrollbar);

			float legendWidth = 270.0f;
			float availableWidth = ImGui::GetContentRegionAvail().x;
			float graphWidth = std::max(50.0f, availableWidth - legendWidth - 10.0f);

			ImVec2 graphPos = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##graph", ImVec2(graphWidth, graphHeight));

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// Draw background.
			drawList->AddRectFilled(graphPos, ImVec2(graphPos.x + graphWidth, graphPos.y + graphHeight), IM_COL32(15, 15, 15, 255), 4.0f);

			char scaleLabel[32];
			snprintf(scaleLabel, sizeof(scaleLabel), "%.2f ms", smoothMax);
			drawList->AddText(
				ImVec2(graphPos.x + 4, graphPos.y + 2),
				IM_COL32(200, 200, 200, 255),
				scaleLabel
			);

			// Render segments.
			float barWidth = graphWidth / 128.0f;
			for (int f = 0; f < 128; f++)
			{
				// Use the defined graphHeight
				float y = graphPos.y + graphHeight;
				for (auto const& [name, h] : mPassHistories)
				{
					int idx = (h.offset - 128 + f) % 128;
					if (idx < 0) idx += 128;

					// Scale by maxMs and graphHeight
					float h_val = std::min((h.history[idx] / smoothMax) * graphHeight, y - graphPos.y);

					drawList->AddRectFilled(ImVec2(graphPos.x + f * barWidth, y - h_val),
						ImVec2(graphPos.x + (f + 1) * barWidth - 1, y),
						h.color);
					y -= h_val;
				}
			}

			// 2.3. Render Legend.
			ImGui::SameLine();
			ImGui::BeginGroup();
			for (auto const& [name, h] : mPassHistories)
			{
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(h.color), "%.2fms", h.lastValidMs);
				ImGui::SameLine();
				ImGui::TextUnformatted(name.c_str());
			}
			ImGui::EndGroup();

			ImGui::EndChild();
		}

		// Scene metrics.
		{
			auto formatNumber = [](uint32_t n, char* buf, size_t bufSize) {
				char tmp[32];
				snprintf(tmp, sizeof(tmp), "%u", n);
				int len = (int)strlen(tmp);
				int commas = (len - 1) / 3;
				int outLen = len + commas;
				buf[outLen] = '\0';
				int i = len - 1;
				int j = outLen - 1;
				int count = 0;
				while (i >= 0) {
					if (count > 0 && count % 3 == 0)
						buf[j--] = ',';
					buf[j--] = tmp[i--];
					count++;
				}
				};

			char entitiesBuf[32], trianglesBuf[32], meshletsBuf[32];
			formatNumber(profInfo.sceneInfo.entities, entitiesBuf, sizeof(entitiesBuf));
			formatNumber(profInfo.sceneInfo.maxLodTriangles, trianglesBuf, sizeof(trianglesBuf));
			formatNumber(profInfo.sceneInfo.maxLodMeshlets, meshletsBuf, sizeof(meshletsBuf));

			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Entities  ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", entitiesBuf);

			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Triangles ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", trianglesBuf);

			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Meshlets  ");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", meshletsBuf);

			ImGui::Separator();
		}

		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(4);
	}

	void uiRenderer::renderAccumAndRevealImages()
	{
		// Style
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

		char title[64];
		snprintf(title, sizeof(title), "Color/depth attachments");

		ImGui::SetNextWindowPos(ImVec2(0, 300), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiCond_Once);

		ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoScrollbar);

		float padding = ImGui::GetStyle().ItemSpacing.x;
		float imageWidth = (ImGui::GetContentRegionAvail().x - padding * 2.0f) / 3.0f;

		float textOverhead = ImGui::GetTextLineHeightWithSpacing();
		float imageHeight = ImGui::GetContentRegionAvail().y - textOverhead;

		if (imageHeight < 10.0f) imageHeight = 10.0f;

		ImVec2 imageSize = ImVec2(imageWidth, imageHeight);

		ImGui::BeginGroup();
		ImGui::Text("Accumulation image");
		ImGui::Image(
			(ImTextureID)mImGuiDescroptorSets[0],
			imageSize
		);
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("Reveal image");
		ImGui::Image(
			(ImTextureID)mImGuiDescroptorSets[1],
			imageSize
		);
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::Text("Depth image");
		ImGui::Image(
			(ImTextureID)mImGuiDescroptorSets[2],
			imageSize
		);
		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(4);
	}

	void uiRenderer::renderHzbImages()
	{
		const size_t baseAttachmentCount = 3;
		if (mImGuiDescroptorSets.size() <= baseAttachmentCount)
			return;

		size_t hzbMipCount = mImGuiDescroptorSets.size() - baseAttachmentCount;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

		ImGui::SetNextWindowPos(ImVec2(0, 600), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(550, 300), ImGuiCond_Once);

		ImGui::Begin("HZB chain", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

		float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
		float textOverhead = ImGui::GetTextLineHeightWithSpacing();

		float imageHeight = ImGui::GetContentRegionAvail().y - textOverhead - ImGui::GetStyle().ScrollbarSize;
		if (imageHeight < 10.0f) imageHeight = 10.0f;

		ImVec2 baseImageSize = ImVec2(imageHeight, imageHeight);

		for (size_t i = 0; i < hzbMipCount; ++i)
		{
			size_t descriptorIndex = baseAttachmentCount + i;

			ImGui::BeginGroup();

			ImGui::Text("Mip %d", static_cast<int>(i));

			ImGui::Image(
				(ImTextureID)mImGuiDescroptorSets[descriptorIndex],
				baseImageSize
			);

			ImGui::EndGroup();

			if (i < hzbMipCount - 1)
				ImGui::SameLine();
		}

		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(1);
	}
}