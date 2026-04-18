project "imgui"
	kind "StaticLib"
	language "C++"
   warnings "off"
   architecture "x64"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   	filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }
      local vulkanSDK = os.getenv("VK_SDK_PATH")
      if vulkanSDK then
         libdirs { os.getenv("VK_SDK_PATH") .. "/Lib" }
         includedirs { os.getenv("VK_SDK_PATH") .. "/Include" }
         links       { "vulkan-1" }

         includedirs { "../vendor/vk-bootstrap/src" }
         links       { "vk-bootstrap" }

         includedirs { "../vendor/vkma/include" }
      else
         error("VK_SDK_PATH environment variable is not set, install vulkanSDK or add VK_SDK_PATH to ENV.")
      end

   	includedirs
   	{
   		"imgui",
   		"glfw/include"
   	}

	files
	{
		"imgui/imconfig.h",
		"imgui/imgui.h",
		"imgui/imgui.cpp",
		"imgui/imgui_draw.cpp",
		"imgui/imgui_internal.h",
		"imgui/imgui_tables.cpp",
		"imgui/imgui_widgets.cpp",
		"imgui/imstb_rectpack.h",
		"imgui/imstb_textedit.h",
		"imgui/imstb_truetype.h",
		"imgui/backends/imgui_impl_glfw.h",
		"imgui/backends/imgui_impl_glfw.cpp",
		"imgui/backends/imgui_impl_vulkan.cpp",
		"imgui/backends/imgui_impl_vulkan.cpp",
		"imgui/imgui_demo.cpp"
	}

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE", "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST", "_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR" }
       runtime "Release"
       optimize "On"
       symbols "Off"
