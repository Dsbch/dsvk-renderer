project "engine"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

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
      "../vendor/glad/include",
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
      "glad", 
      "glm",
      "opengl32.lib",   
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