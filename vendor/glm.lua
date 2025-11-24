project "glm"
   kind "StaticLib"
   language "C"
   architecture "x64"
   warnings "off"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")
   
   includedirs { "glm" }   

   files
   {
      "glm/glm/**"
   }
    
   filter "system:windows"
      systemversion "latest"
      staticruntime "On"

      defines 
      { 
         "_GLM_WIN32",
         "_CRT_SECURE_NO_WARNINGS"
      }

   filter "configurations:Debug"
      defines { "DEBUG" }
      runtime "Debug"
      symbols "on"

   filter "configurations:Release"
      defines { "NDEBUG" }
      runtime "Release"
      optimize "on"