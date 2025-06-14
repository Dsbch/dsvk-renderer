workspace "dsengine"
   configurations { "Debug", "Release", "Dist" }
   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   newoption {
      trigger     = "gfxapi",
      value       = "API",
      description = "Choose a particular 3D API for rendering",
      default     = "opengl",
      category    = "Build Options",
      allowed = {
         { "opengl" },
         { "vulkan" },
      }
   }

   newoption {
      trigger     = "osio",
      value       = "API",
      description = "Choose a particular osio API for os",
      default     = "winapi",
      category    = "Build Options",
      allowed = {
         { "winapi" },
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
   include "vendor/glad.lua"
   include "vendor/glm.lua"
   include "editor/editor.lua"
   include "sandbox/sandbox.lua"
