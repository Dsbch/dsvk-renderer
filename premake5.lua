workspace "dsengine"
   configurations { "Debug", "Release", "Dist" }
   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   newoption {
      trigger     = "gfxapi",
      value       = "API",
      description = "Choose a particular 3D API for rendering",
      default     = "vulkan",
      category    = "Build Options",
      allowed = {
         { "vulkan" },
      }
   }

   newoption {
      trigger     = "mode",
      value       = "work mode",
      description = "Choose a particular build mode",
      default     = "editor",
      category    = "Build Options",
      allowed = {
         { "editor" },
         { "sandbox" },
      }
   }

   filter { "options:mode=sandbox" }
      startproject "sandbox"

   filter { "options:mode=editor" }
      startproject "editor"

   include "engine/engine.lua"
   include "vendor/spdlog.lua"   
   include "editor/editor.lua"
   include "sandbox/sandbox.lua"
   include "vendor/vk-bootstrap.lua"
   include "vendor/meshoptimizer.lua"
   include "vendor/glfw.lua"
