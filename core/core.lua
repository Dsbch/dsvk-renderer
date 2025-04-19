project "core"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   cppdialect "C++17"
   
   targetdir "bin/%{cfg.buildcfg}"
   objdir "bin-int/%{cfg.buildcfg}"
    
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

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"