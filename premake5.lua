workspace "engine"
   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   configurations { "Debug", "Release" }

   startproject "engine"

   include "engine/engine.lua"
   include "core/core.lua"
   include "vendor/spdlog.lua"
   include "vendor/glad.lua"
   include "vendor/glm.lua"
