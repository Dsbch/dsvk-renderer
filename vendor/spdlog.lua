project "spdlog"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   warnings "off"
   
   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")
    
   defines 
   {
      "SPDLOG_COMPILED_LIB"
   }

   includedirs 
   {
      "spdlog/include",
   }

   files
   {
      "spdlog/src/**.cpp",
   }

   filter "system:windows"
        buildoptions { "/utf-8" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"