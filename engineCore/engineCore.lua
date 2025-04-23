project "engineCore"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")
    
   includedirs
   {
      "src",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/spdlog/include",
   }

   files
   {
      "src/**.cpp",
      "src/**.h",
   }

   links
   {
      "spdlog",
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