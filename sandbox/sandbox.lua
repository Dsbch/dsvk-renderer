project "sandbox"
   kind "ConsoleApp"
   language "C++"
   architecture "x64"
   cppdialect "C++20"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }

   files
   {
      "src/**.cpp",
      "src/**.h",
   }

   fatalwarnings
   {
    "All",
   }

   includedirs {
      "src",
      "../engine/src",
      "../vendor/spdlog/include",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/entt/src",
      "../vendor/glm",
   }

   links
   {
      "engine",
   }

   filter "system:windows"
        buildoptions { "/utf-8" }
        defines 
        { 
           "_GLM_WIN32",
           "_CRT_SECURE_NO_WARNINGS"
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