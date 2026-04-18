project "editor"
   kind "ConsoleApp"
   language "C++"
   architecture "x64"
   cppdialect "C++20"
   conformancemode "On"
   usestandardpreprocessor "On"
   externalwarnings "Off"

   filter { "options:gfxapi=vulkan" }
      defines { "VULKAN" }
   
   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   files {
      "src/**.hpp",
      "src/**.cpp",
      "src/**.c",
      "src/**.h",
   }

   externalincludedirs
   {
      "../engine/src",
      "../vendor/spdlog/include",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/entt/src",
      "../vendor/glm",
      "../vendor/coost/include",
   }

   includedirs 
   {
      "src",
    }

   links 
   { 
      "engine",
      "spdlog",
      "meshoptimizer",
      "imgui",
      "glfw",
      "basis_universal",
      "coost",
   }

   fatalwarnings
   {
    "All",
   }

   filter "system:windows"
       buildoptions { "/utf-8" }
       defines 
       { 
         "WINDOWS",
         "_GLM_WIN32",
         "_CRT_SECURE_NO_WARNINGS",
         "NOMINMAX"
       }
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