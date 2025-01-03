project "spdlog"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   
   targetdir "bin/%{cfg.buildcfg}"
   objdir "bin-int/%{cfg.buildcfg}"
    
   defines {
      "SPDLOG_COMPILED_LIB"
   }

   includedirs {
      "spdlog/include",
   }

   files
   {
      "spdlog/src/**.cpp",
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"