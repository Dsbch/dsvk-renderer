project "meshoptimizer"
   kind "StaticLib"
   language "C++"
   architecture "x64"
   
   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   includedirs {
      "meshoptimizer/src",
   }

   files
   {
      "meshoptimizer/src/**.cpp",
   }

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