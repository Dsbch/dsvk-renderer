project "application"
   kind "WindowedApp"
   language "C++"
   architecture "x64"

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
      "../vendor/stb/stb_image/include",
      "../vendor/json",
   }

   links { 
      "glad", 
      "spdlog",
      "glm",
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"