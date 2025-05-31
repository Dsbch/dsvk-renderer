workspace "dsengine"

   newoption {
      trigger     = "gfxapi",
      value       = "API",
      description = "Choose a particular 3D API for rendering",
      default     = "opengl",
      category    = "Build Options",
      allowed = {
         { "opengl", "OpenGL" },
      }
   }

   newoption {
      trigger     = "osio",
      value       = "API",
      description = "Choose a particular osio API for os",
      default     = "winapi",
      category    = "Build Options",
      allowed = {
         { "winapi", "WinApi", "Win32Api" },
      }
   }

   outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

   configurations { "Debug", "Release", "Dist" }

   startproject "editor"

   include "editor/editor.lua"
   include "engine/engine.lua"
   include "vendor/spdlog.lua"
   include "vendor/glad.lua"
   include "vendor/glm.lua"
