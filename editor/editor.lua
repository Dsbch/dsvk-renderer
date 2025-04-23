project "editor"
   kind "WindowedApp"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   files {
      "src/**.hpp",
      "src/**.cpp",
      "src/**.c",
      "src/**.h",
   }

   pchheader ("pch.h")
   pchsource ("src/pch.cpp")

   includedirs 
   {
      "src",
      "../engineCore/src",
      "../vendor/glad/include",
      "../vendor/glm",
      "../vendor/stb",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/spdlog/include",
   }

   links { 
      "glad", 
      "glm",
      "opengl32.lib",
      "engineCore",
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