-- premake5.lua
workspace "engine"
   location "%{_ACTION}"
   configurations { "Debug", "Release" }

   include "src/application.lua"
   include "vendor/glad.lua"
   include "vendor/spdlog.lua"
   include "vendor/glm.lua"
