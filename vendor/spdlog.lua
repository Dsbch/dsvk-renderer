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