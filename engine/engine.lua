project "engine"
   kind "WindowedApp"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

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
      "../core/src",
      "src",
      "../vendor/glad/include",
      "../vendor/glm",
      "../vendor/stb",
      "../core/vendor/json",
      "../core/vendor/json/single_include",
      "../core/vendor/spdlog/include",
   }

   links { 
      "glad", 
      "glm",
      "opengl32.lib",
      "core",
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"