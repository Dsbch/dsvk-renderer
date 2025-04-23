project "glad"
   kind "StaticLib"
   language "C"
   architecture "x64"
   
   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   files
   {
      "glad/include/**.h",
      "glad/src/**.c",
   }

   includedirs { 
      "glad/include", 
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"