#include <pch.h>
#include "ui.h"
#include "helper.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace engine
{
	error vulkanUI::init(GLFWwindow* wnd, VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance, uint32_t queueFamily, VkQueue queue, VkFormat colorAttachmentFormat)
	{
		mColorAttachmentFormat = colorAttachmentFormat;

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

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(wnd, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = instance;
		initInfo.PhysicalDevice = physicalDevice;
		initInfo.Device = device;
		initInfo.QueueFamily = queueFamily;
		initInfo.Queue = queue;
		initInfo.DescriptorPoolSize = 8;
		initInfo.MinImageCount = 2;
		initInfo.ImageCount = 3;
		initInfo.PipelineInfoMain.Subpass = 0;
		initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, .colorAttachmentCount = 1, .pColorAttachmentFormats = &mColorAttachmentFormat };;
		initInfo.CheckVkResultFn = logResult;
		initInfo.UseDynamicRendering = true;

		bool result = ImGui_ImplVulkan_Init(&initInfo);
		if (!result)
			return error{ "can't init UI" };

		return error();
	}

	error vulkanUI::destroy()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		return {};
	}

	error vulkanUI::onRender(VkCommandBuffer cmd, profilingInfo profInfo)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Add calls to imgui here.
		renderProfilingInfo(profInfo);

		ImGui::Render();

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

		return {};
	}

	void vulkanUI::renderProfilingInfo(profilingInfo profInfo)
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
		snprintf(title, sizeof(title), "Profiler [%.2ffps]###profiler",
			profInfo.renderingInfo.fps);

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
		
		ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoScrollbar);

		// FPS.
		{
			static float fpsHistory[128] = {};
			static int offset = 0;

			static auto lastUpdate = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(now - lastUpdate).count();

			if (elapsed >= 60.0f)
			{
				fpsHistory[offset] = profInfo.renderingInfo.fps;
				offset = (offset + 1) % 128;
				lastUpdate = now;
			}

			char overlay[64];
			snprintf(
				overlay,
				sizeof(overlay),
				"%.2f fps, %.2f ms, deltaTime %.4f ms",
				profInfo.renderingInfo.fps,
				1000.0f / profInfo.renderingInfo.fps,
				profInfo.renderingInfo.deltaTime
			);
			ImGui::PlotLines(
				"##fps",
				fpsHistory,
				128,
				offset,
				overlay,
				0.0f, 200.0f,
				ImVec2(ImGui::GetContentRegionAvail().x, 80)
			);

			ImGui::Separator();

			ImGui::PopStyleVar(3);
			ImGui::PopStyleColor(4);
		}

		// Render passes.
		{
			static const uint32_t passCount = 4;
			static float history[passCount][128] = {};
			static int offset = 0;
			static float lastValidMs[passCount] = {};

			static auto lastUpdate = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			float elapsed = std::chrono::duration<float, std::milli>(now - lastUpdate).count();

			if (elapsed >= 60.0f)
			{
				history[0][offset] = profInfo.renderingInfo.opaquePass;
				history[1][offset] = profInfo.renderingInfo.transperentPass;
				history[2][offset] = profInfo.renderingInfo.compositePass;
				history[3][offset] = profInfo.renderingInfo.uiPass;
				offset = (offset + 1) % 128;
				lastUpdate = now;
			}

			struct PassInfo {
				const char* name;
				ImU32 color;
			};
			static PassInfo passes[] = {
				{ "Opaque Pass",      IM_COL32(255, 150, 0,   255) },
				{ "Transparent Pass", IM_COL32(0,   255, 128, 255) },
				{ "Composite Pass",   IM_COL32(255, 50,  50,  255) },
				{ "UI Pass",          IM_COL32(180, 0,   255, 255) },
			};

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);

			float legendWidth = 200.0f;
			float graphWidth = ImGui::GetContentRegionAvail().x - legendWidth - 8.0f;

			if (graphWidth < 50.0f)
			{
				ImGui::InvisibleButton("##graph", ImGui::GetContentRegionAvail());
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(1);
				ImGui::End();
				return;
			}

			ImVec2 graphSize = ImVec2(graphWidth, 100);
			ImVec2 graphPos = ImGui::GetCursorScreenPos();
			ImDrawList* dl = ImGui::GetWindowDrawList();
			float graphBottom = graphPos.y + graphSize.y;
			float graphRightX = graphPos.x + graphSize.x;

			dl->PushClipRect(graphPos, ImVec2(graphRightX, graphBottom), true);

			dl->AddRectFilled(graphPos,
				ImVec2(graphRightX, graphBottom),
				IM_COL32(15, 15, 15, 255), 4.0f);

			float dynamicMax = 1.0f;
			for (int f = 0; f < 128; f++)
			{
				float total = 0.0f;
				for (int p = 0; p < passCount; p++)
					total += history[p][f];
				dynamicMax = std::max(dynamicMax, total);
			}
			static float smoothMax = 1.0f;
			smoothMax += (dynamicMax - smoothMax) * 0.05f;
			float maxMs = smoothMax;

			ImDrawList* fgDl = ImGui::GetForegroundDrawList();
			char scaleLabel[32];
			snprintf(scaleLabel, sizeof(scaleLabel), "%.1fms", maxMs);
			fgDl->AddText(
				ImVec2(graphPos.x + 2, graphPos.y + 2),
				IM_COL32(255, 255, 255, 255),
				scaleLabel
			);

			float barGap = 1.0f;
			float segmentGap = 1.0f;
			float rounding = 2.0f;
			float barWidth = std::ceil(graphSize.x / 128.0f);
			float drawWidth = barWidth - barGap;

			for (int f = 0; f < 128; f++)
			{
				int   idx = (offset + f) % 128;
				float x = graphPos.x + f * barWidth;
				float y = graphBottom;

				for (int p = 0; p < passCount; p++)
				{
					float h = std::min((history[p][idx] / maxMs) * graphSize.y, y - graphPos.y);
					if (h < 1.0f) { y -= h; continue; }

					dl->AddRectFilled(
						ImVec2(x, y - h + segmentGap),
						ImVec2(x + drawWidth, y),
						passes[p].color, rounding
					);
					y -= h;
				}
			}

			dl->PopClipRect();

			int lastIdx = (offset - 1 + 128) % 128;
			bool allValid = true;
			for (int p = 0; p < passCount; p++)
				if (history[p][lastIdx] == 0.0f) { allValid = false; break; }

			if (allValid)
				for (int p = 0; p < passCount; p++)
					lastValidMs[p] = history[p][lastIdx];

			float legendStartX = graphRightX + 8.0f;
			float legendEndX = legendStartX + legendWidth;
			float textHeight = ImGui::GetTextLineHeight();
			float lineHeight = graphSize.y / passCount;

			dl->PushClipRect(
				ImVec2(legendStartX, graphPos.y),
				ImVec2(legendEndX, graphBottom),
				true
			);

			for (int i = passCount - 1; i >= 0; i--)
			{
				float ms = lastValidMs[i];
				int   slot = (passCount - 1) - i;
				float labelY = graphPos.y + slot * lineHeight + (lineHeight - textHeight) * 0.5f;

				dl->AddRectFilled(
					ImVec2(legendStartX, labelY + 1),
					ImVec2(legendStartX + 8.0f, labelY + textHeight - 1),
					passes[i].color, 2.0f
				);

				char label[64];
				snprintf(label, sizeof(label), "[%.2fms] %s", ms, passes[i].name);
				dl->AddText(
					ImVec2(legendStartX + 12.0f, labelY),
					passes[i].color,
					label
				);
			}

			dl->PopClipRect();

			ImGui::InvisibleButton("##graph", ImVec2(graphSize.x + legendWidth + 8.0f, graphSize.y));
			ImGui::Separator();

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(1);
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
	}
}