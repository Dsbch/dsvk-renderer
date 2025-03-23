project "application"
   kind "WindowedApp"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

   targetdir "bin/%{cfg.buildcfg}"
   objdir "bin-int/%{cfg.buildcfg}"

   files {
      "**.hpp",
      "**.cpp",
      "**.c",
      "**.h",
   }

   pchheader ("pch.h")
   pchsource ("include/pch.cpp")

   includedirs { 
      "include",
      "../vendor/spdlog/include", 
      "../vendor/glad/include",
      "../vendor/glm",
      "../vendor/json",
      "../vendor/json/single_include",
      "../vendor/stb",
   }

   links { 
      "glad", 
      "spdlog",
      "glm",
      "opengl32.lib",
   }

   postbuildcommands {
      "{copy} ../assets ."
   }


   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"