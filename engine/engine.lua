project "engine"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++20"
   conformancemode "On"
   usestandardpreprocessor "On"
   externalwarnings "Off"

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

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   pchheader ("pch.h")
   pchsource ("src/pch.cpp")
    
   externalwarnings "Off"
   
   externalincludedirs
   {
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/spdlog/include",
      "../vendor/glm",
      "../vendor/stb",
      "../vendor/entt/src",
      "../vendor/meshoptimizer/src",
      "../vendor/cgltf",
      "../vendor/glfw/include",
      "../vendor/imgui",
      "../vendor/basis_universal/transcoder",
   }

   includedirs
   {
      "src",
   }

   files
   {
      "src/**.cpp",
      "src/**.h",
   }

   links
   {
      "spdlog",
      "meshoptimizer",
      "imgui",
      "glfw",
      "basis_universal",
   }

   fatalwarnings 
   { 
      "All" 
   }

   filter "system:windows"
         buildoptions { "/utf-8" }
         defines 
         { 
            "_GLM_WIN32",
            "_CRT_SECURE_NO_WARNINGS",
         }

   filter { "files:assets/shaders/**" }
        buildaction "None"

   filter "system:windows"
       systemversion "latest"
       defines { }

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