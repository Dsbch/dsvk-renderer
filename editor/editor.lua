project "editor"
   kind "ConsoleApp"
   language "C++"
   architecture "x64"
   cppdialect "C++20"
   
   filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }
   
   filter { "options:osio=winapi" }
      defines { "WIN32API" }

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   files {
      "src/**.hpp",
      "src/**.cpp",
      "src/**.c",
      "src/**.h",
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

   flags
   {
    "FatalWarnings",
   }

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

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