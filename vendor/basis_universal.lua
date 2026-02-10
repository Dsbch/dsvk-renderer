project "basis_universal"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	warnings "off"
	architecture "x64"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin/inter/" .. outputdir .. "/%{prj.name}")
	
	includedirs
	{
		"basis_universal/transcoder",
	}
	
	files
	{
		"basis_universal/transcoder/basisu_transcoder.cpp",
		"basis_universal/zstd/zstd.c",
	}
	
   filter "system:windows"
       systemversion "latest"
       defines { "_CRT_SECURE_NO_WARNINGS" }

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
