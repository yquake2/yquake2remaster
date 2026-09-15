---
name: vulkan-develop
description: A skill for developing the Vulkan renderer in Yamagi Quake II Remaster. Use when making changes to Vulkan-specific logic, rendering pipelines, buffers, or shaders.
---

# Vulkan Renderer Development Guidelines

When developing, debugging, or modifying the Vulkan renderer in Yamagi Quake II Remaster, follow these key workflows and rules:

## 1. Vulkan Validation Layers (`r_validation`)
Whenever you make changes to Vulkan-specific rendering logic, command buffers, descriptor sets, memory management, or pipeline creation:
- Always test your changes by running Quake II with the Vulkan validation cvar enabled:
  ```bash
  +set r_validation 2
  ```
  *(Or set `r_validation` to `2` in your configuration / console)*.
- Inspect logs for any validation warnings or errors to ensure there are no resource leaks, synchronization hazards, or invalid API usage.

## 2. Shader Compilation (`shaders.sh`)
The Vulkan renderer uses GLSL vertex (`.vert`) and fragment (`.frag`) shaders located in `stuff/shaders/`. These are compiled into SPIR-V C-source arrays using `glslangValidator`.
If your changes involve modifying any shader source files:
- You must recompile the updated shaders by running the shader build script from the `stuff/shaders/` directory:
  ```bash
  cd stuff/shaders && ./shaders.sh
  ```
- Ensure that the compiled SPIR-V C headers under `src/client/refresh/vk/spirv/` are correctly updated before rebuilding and running the project.

## 3. General Rules & Conventions
- Adhere strictly to the existing codebase conventions and styling guidelines (`.agents/docs/code_style.md`).
- Ensure robust error handling for Vulkan calls (`VkResult` checks).
- Verify cross-platform compatibility where applicable.
