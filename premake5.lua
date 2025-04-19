workspace "engine"
   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   configurations { "Debug", "Release" }

   startproject "engine"

   include "engine/engine.lua"
   include "core/core.lua"
   include "core/vendor/spdlog.lua"
   include "engine/vendor/glad.lua"
   include "engine/vendor/glm.lua"
