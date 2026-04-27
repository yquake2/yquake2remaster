# Vulkan Fog Shader Implementation

This directory contains GLSL shader source files for the Vulkan renderer that now include support for fog effects.

## Shader Files Updated for Fog Support

The following shaders have been modified to include fog parameters:

### Vertex Shaders:
- `model.vert` - Model rendering with fog depth calculation
- `polygon.vert` - Basic polygon rendering with fog depth
- `polygon_lmap.vert` - Lightmap polygon rendering with fog depth
- `polygon_warp.vert` - Warping surface (water) rendering with fog depth

### Fragment Shaders:
- `model.frag` - Model fragment shader with fog application
- `basic.frag` - Basic texture shader with fog application
- `polygon_lmap.frag` - Lightmap fragment shader with fog application

## Fog Parameter Structure

All fragment shaders now receive fog parameters via a UBO at descriptor set 3, binding 0:

```glsl
layout(set = 3, binding = 0) uniform FogParameters
{
    vec4 fogColor; // RGB + density in .w
    vec4 heightfog_start; // RGB + start distance in .w
    vec4 heightfog_end; // RGB + end distance in .w
    float heightfog_density;
    float heightfog_falloff;
    float _pad1;
    float _pad2;
} fog;
```

## Fog Calculation

Fog is applied using quadratic exponential falloff:

```glsl
if (fog.fogColor.w > 0.0)
{
    float d = fog.fogColor.w * depth;
    float fogFactor = 1.0 - exp(-(d * d));
    outColor.rgb = mix(outColor.rgb, fog.fogColor.rgb, fogFactor);
}
```

## Compiling Shaders to SPIR-V

To recompile the GLSL shaders to SPIR-V format, you need the Vulkan SDK installed with `glslangValidator`:

```bash
cd stuff/shaders
./shaders.sh
```

This will generate SPIR-V bytecode files in `src/client/refresh/vk/spirv/`.

### Installing glslangValidator

On Debian/Ubuntu:
```bash
sudo apt-get install glslang-tools
```

Or download from the official Vulkan SDK: https://vulkan.lunarg.com/sdk/home

## Implementation Notes

- Depth is calculated per-vertex in world space
- Fog blending occurs in the fragment shader after all other color calculations
- Fog parameters are passed from the game server via `r_newrefdef.fog` structure
- The fog density of 0 disables fog rendering (no performance penalty)
- Supports both distance-based and height-based fog (height fog parameters available but not yet used in calculation)
