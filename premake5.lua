workspace "dsengine"

   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   configurations { "Debug", "Release", "Dist" }

   startproject "editor"

   include "editor/editor.lua"
   include "engineCore/engineCore.lua"
   include "vendor/spdlog.lua"
   include "vendor/glad.lua"
   include "vendor/glm.lua"
