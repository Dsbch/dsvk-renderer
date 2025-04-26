project "editor"
   kind "WindowedApp"
   language "C++"
   architecture "x64"
   cppdialect "C++17"

   targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
   objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")

   files {
      "src/**.hpp",
      "src/**.cpp",
      "src/**.c",
      "src/**.h",
   }

   includedirs {
      "src",
      "../engine/src",
      "../vendor/glm",
      "../vendor/spdlog/include",
      "../vendor/json",
      "../vendor/json/single_include",
    }

   links 
   { 
      "engine",
   }

    filter "action:vs2022"
        if _OPTIONS["clang"] then
            toolset "clang"
            print("Using clang compiler editor.lua")
        end

       
   flags
   {
    "FatalWarnings",
   }

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

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