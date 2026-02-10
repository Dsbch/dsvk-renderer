project "vk-bootstrap"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++20"
   warnings "off"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }
      local vulkanSDK = os.getenv("VK_SDK_PATH")
      if vulkanSDK then
         includedirs { os.getenv("VK_SDK_PATH") .. "/Include" }
      else
         error("VK_SDK_PATH environment variable is not set, install vulkanSDK then add VK_SDK_PATH to ENV.")
      end
   
   includedirs { "vk-bootstrap/src" }   

   files
   {
      "vk-bootstrap/src/**"
   }
    
   filter "system:windows"
      systemversion "latest"

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