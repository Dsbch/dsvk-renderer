project "engine"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++20"

   filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }
      local vulkanSDK = os.getenv("VK_SDK_PATH")
      if vulkanSDK then
         libdirs { os.getenv("VK_SDK_PATH") .. "/Lib" }
         includedirs { os.getenv("VK_SDK_PATH") .. "/Include" }
         links       { "vulkan-1" }
      else
         error("VK_SDK_PATH environment variable is not set, install vulkanSDK or add VK_SDK_PATH to ENV.")
      end

   filter { "options:gfxapi=opengl" }
      defines { "OPENGL" }   
      includedirs { "../vendor/glad/include" }
      links { "opengl32", "glad" }      

   filter { "options:osio=winapi" }
      defines { "WIN32API" }

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   pchheader ("pch.h")
   pchsource ("src/pch.cpp")
    
   includedirs
   {
      "src",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/spdlog/include",
      "../vendor/glm",
      "../vendor/stb",
      "../vendor/entt/src",
   }

   files
   {
      "src/**.cpp",
      "src/**.h",
   }

   links
   {
      "spdlog",
      "glm",
   }

   flags
   {
    "FatalWarnings",
   }

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"