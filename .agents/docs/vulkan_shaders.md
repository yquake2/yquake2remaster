# Vulkan Shaders and Push Constants Specification

This document specifies all shader combinations, pipeline layouts, and push constant memory layouts used by the Vulkan renderer in Yamagi Quake II Remaster.

---

## 1. Global Push Constant Architecture

All graphics pipelines configure their `VkPipelineLayout` with two push constant ranges (defined in [`src/client/refresh/vk/vk_pipeline.c`](../../src/client/refresh/vk/vk_pipeline.c#L174-L186) and [`src/client/refresh/vk/header/qvk.h`](../../src/client/refresh/vk/header/qvk.h#L224-L230)):

```c
#define PUSH_CONSTANT_VERTEX_SIZE   17  /* 17 floats = 68 bytes */
#define PUSH_CONSTANT_FRAGMENT_SIZE 11  /* 11 floats = 44 bytes */
#define PUSH_CONSTANT_FOG_INDEX     3   /* Offset 68 + (3 * 4) = 80 */
#define PUSH_CONSTANT_POSTPROCESS_INDEX 1 /* Offset 68 + (1 * 4) = 72 */
```

### Pipeline Layout Push Constant Ranges

| Stage | Start Offset | Configured Size (`VkPushConstantRange`) | Description |
|---|---|---|---|
| **`VK_SHADER_STAGE_VERTEX_BIT`** | **`0`** | **`68 bytes`** (17 floats)<br>*(**`80 bytes`** / 20 floats for sprite pipelines)* | MVP / VP transformation matrix (+ sprite color/alpha) |
| **`VK_SHADER_STAGE_FRAGMENT_BIT`** | **`68`** | **`44 bytes`** (11 floats) | Gamma, direct-render postprocess, fog, warp params |

> [!NOTE]
> The fragment stage constants start at byte offset **68** (`PUSH_CONSTANT_VERTEX_SIZE * sizeof(float)`). This separation guarantees that fragment state (pushed during `QVk_BindPipeline` or draw setup) is never clobbered by vertex push constants.

---

## 2. Global Fragment Push Constant Layout Map

Fragment shaders share common layout conventions based on the 11-float fragment buffer (spanning offsets `68` through `111`):

| Relative Index | Byte Offset | Type | Name | Shaders Using Field |
|---|---|---|---|---|
| `0` | **`68`** | `float` | `gamma` | [`basic.frag`](../../stuff/shaders/basic.frag#L6) |
| `0` | **`68`** | `float` | `postprocess` | [`postprocess.frag`](../../stuff/shaders/postprocess.frag#L6) |
| `0` | **`68`** | `float` | `time` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L9) |
| `1` | **`72`** | `float` | `postprocess` | [`basic.frag`](../../stuff/shaders/basic.frag#L7), [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag#L7), [`model.frag`](../../stuff/shaders/model.frag#L7), [`point_particle.frag`](../../stuff/shaders/point_particle.frag#L7), [`polygon_lmap.frag`](../../stuff/shaders/polygon_lmap.frag#L7) |
| `1` | **`72`** | `float` | `gamma` | [`postprocess.frag`](../../stuff/shaders/postprocess.frag#L7) |
| `1` | **`72`** | `float` | `scale` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L10) |
| `2` | **`76`** | `float` | `postGamma` | [`basic.frag`](../../stuff/shaders/basic.frag#L8), [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag#L8), [`model.frag`](../../stuff/shaders/model.frag#L8), [`point_particle.frag`](../../stuff/shaders/point_particle.frag#L8), [`polygon_lmap.frag`](../../stuff/shaders/polygon_lmap.frag#L8) |
| `2` | **`76`** | `float` | `scrWidth` | [`postprocess.frag`](../../stuff/shaders/postprocess.frag#L8), [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L11) |
| `3` | **`80`** | `vec4` / `float` | `fogColor.r` / `scrHeight` | `fogColor` in standard passes; `scrHeight` in postprocess/warp |
| `4` | **`84`** | `float` | `fogColor.g` / `offsetX` | `fogColor` in standard passes; `offsetX` in postprocess/warp |
| `5` | **`88`** | `float` | `fogColor.b` / `offsetY` | `fogColor` in standard passes; `offsetY` in postprocess/warp |
| `6` | **`92`** | `float` | `fogColor.a` / `pixelSize` | `fogColor.a` (density) in standard passes; `pixelSize` in warp |
| `7` | **`96`** | `float` | `refdefX` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L16) |
| `8` | **`100`** | `float` | `refdefY` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L17) |
| `9` | **`104`** | `float` | `refdefWidth` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L18) |
| `10` | **`108`** | `float` | `refdefHeight` | [`world_warp.frag`](../../stuff/shaders/world_warp.frag#L19) |

---

## 3. Shader Combinations & Pipeline Details

All pipelines are initialized in [`src/client/refresh/vk/vk_common.c`](../../src/client/refresh/vk/vk_common.c#L1460-L1678):

### 1. Basic Textured Quad
* **Pipelines**: `vk_drawTexQuadPipeline[RP_WORLD]`, `vk_drawTexQuadPipeline[RP_UI]`, `vk_drawTexQuadPipeline[RP_WORLD_WARP]`
* **Vertex Shader**: [`basic.vert`](../../stuff/shaders/basic.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (uses UBO `imageTransform` at Set 1, Binding 0; pushes 68-byte dummy buffer)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`):
    * `layout(offset = 68) float gamma;` (4 bytes)
    * `layout(offset = 72) float postprocess;` (4 bytes)
    * `layout(offset = 76) float postGamma;` (4 bytes)
    * `layout(offset = 80) vec4 fogColor;` (16 bytes)

---

### 2. Basic Tinted Quad
* **Pipelines**: `vk_drawTexQuadTintedPipeline[RP_WORLD]`, `vk_drawTexQuadTintedPipeline[RP_UI]`, `vk_drawTexQuadTintedPipeline[RP_WORLD_WARP]`
* **Vertex Shader**: [`basic_tinted.vert`](../../stuff/shaders/basic_tinted.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (uses UBO `imageTransform` with `tintColor` at Set 1, Binding 0)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 3. Textured Particles
* **Pipelines**: `vk_drawParticlesPipeline`
* **Vertex Shader**: [`particle.vert`](../../stuff/shaders/particle.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 mvpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 4. Point Particles
* **Pipelines**: `vk_drawPointParticlesPipeline`
* **Vertex Shader**: [`point_particle.vert`](../../stuff/shaders/point_particle.vert)
* **Fragment Shader**: [`point_particle.frag`](../../stuff/shaders/point_particle.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 mvpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `24 bytes` (offsets `72`–`95`):
    * `layout(offset = 72) float postprocess;` (4 bytes)
    * `layout(offset = 76) float postGamma;` (4 bytes)
    * `layout(offset = 80) vec4 fogColor;` (16 bytes)

---

### 5. Colored Quad
* **Pipelines**: `vk_drawColorQuadPipeline[RP_WORLD]`, `vk_drawColorQuadPipeline[RP_UI]`, `vk_drawColorQuadPipeline[RP_WORLD_WARP]`
* **Vertex Shader**: [`basic_color_quad.vert`](../../stuff/shaders/basic_color_quad.vert)
* **Fragment Shader**: [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (uses UBO `imageTransform` at Set 0, Binding 0)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `8 bytes` (offsets `72`–`79`):
    * `layout(offset = 72) float postprocess;` (4 bytes)
    * `layout(offset = 76) float postGamma;` (4 bytes)

---

### 6. Untextured Null Model
* **Pipelines**: `vk_drawNullModelPipeline`
* **Vertex Shader**: [`nullmodel.vert`](../../stuff/shaders/nullmodel.vert)
* **Fragment Shader**: [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `8 bytes` (offsets `72`–`79`, same as [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag))

---

### 7. Textured Alias / MD2 Model
* **Pipelines**:
  * `vk_drawModelPipelineFan[RP_WORLD]`, `vk_drawModelPipelineFan[RP_UI]`, `vk_drawModelPipelineFan[RP_WORLD_WARP]`
  * `vk_drawNoDepthModelPipelineFan` (translucent models)
  * `vk_drawLefthandModelPipelineFan` (left-handed view weapon models)
* **Vertex Shader**: [`model.vert`](../../stuff/shaders/model.vert)
* **Fragment Shader**: [`model.frag`](../../stuff/shaders/model.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `24 bytes` (offsets `72`–`95`):
    * `layout(offset = 72) float postprocess;` (4 bytes)
    * `layout(offset = 76) float postGamma;` (4 bytes)
    * `layout(offset = 80) vec4 fogColor;` (16 bytes)

---

### 8. Sprites and Flares
* **Pipelines**: `vk_drawSpritePipeline`, `vk_drawSpriteFlaresPipeline` (additive blending)
* **Vertex Shader**: [`sprite.vert`](../../stuff/shaders/sprite.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `80 bytes` (20 floats: `sizeof(float) * 20`)
  * **Shader Declared**: `68 bytes` (offsets `0`–`67`):
    * `mat4 mvpMatrix;` (64 bytes, offsets `0`–`63`)
    * `float alpha;` (4 bytes, offsets `64`–`67`)
    * *(Note: `vkCmdPushConstants` uploads 4 floats for `spriteColor` at offset 64, which is accommodated by the 80-byte allocation)*
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 9. Alpha-Blended Polygon
* **Pipelines**: `vk_drawPolyPipeline`
* **Vertex Shader**: [`polygon.vert`](../../stuff/shaders/polygon.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 mvpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 10. Lightmapped World Polygon (BSPs)
* **Pipelines**: `vk_drawPolyLmapPipeline`
* **Vertex Shader**: [`polygon_lmap.vert`](../../stuff/shaders/polygon_lmap.vert)
* **Fragment Shader**: [`polygon_lmap.frag`](../../stuff/shaders/polygon_lmap.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `24 bytes` (offsets `72`–`95`):
    * `layout(offset = 72) float postprocess;` (4 bytes)
    * `layout(offset = 76) float postGamma;` (4 bytes)
    * `layout(offset = 80) vec4 fogColor;` (16 bytes)

---

### 11. Warped Polygon (Liquids / Water / Slime / Lava)
* **Pipelines**: `vk_drawPolyWarpPipeline` (translucent), `vk_drawPolySolidWarpPipeline` (opaque)
* **Vertex Shader**: [`polygon_warp.vert`](../../stuff/shaders/polygon_warp.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 12. Laser / Lightning Beams
* **Pipelines**: `vk_drawBeamPipeline`
* **Vertex Shader**: [`beam.vert`](../../stuff/shaders/beam.vert)
* **Fragment Shader**: [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 mvpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `8 bytes` (offsets `72`–`79`, same as [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag))

---

### 13. Skybox
* **Pipelines**: `vk_drawSkyboxPipeline`
* **Vertex Shader**: [`skybox.vert`](../../stuff/shaders/skybox.vert)
* **Fragment Shader**: [`basic.frag`](../../stuff/shaders/basic.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `28 bytes` (offsets `68`–`95`, same as [`basic.frag`](../../stuff/shaders/basic.frag))

---

### 14. Dynamic Light Spheres & Triangle Debug Wireframe
* **Pipelines**: `vk_drawDLightPipeline`, `vk_showTrisPipeline` (`r_showtris`)
* **Vertex Shader**: [`d_light.vert`](../../stuff/shaders/d_light.vert)
* **Fragment Shader**: [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (uses UBO `UniformBufferObject` for `mat4 mvpMatrix` at Binding 0)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `8 bytes` (offsets `72`–`79`, same as [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag))

---

### 15. Entity Shadows
* **Pipelines**: `vk_shadowsPipelineFan`
* **Vertex Shader**: [`shadows.vert`](../../stuff/shaders/shadows.vert)
* **Fragment Shader**: [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: `64 bytes` (offsets `0`–`63`):
    * `mat4 vpMatrix;` (64 bytes)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `8 bytes` (offsets `72`–`79`, same as [`basic_color_quad.frag`](../../stuff/shaders/basic_color_quad.frag))

---

### 16. Underwater World Warp Postprocess
* **Pipelines**: `vk_worldWarpPipeline`
* **Vertex Shader**: [`world_warp.vert`](../../stuff/shaders/world_warp.vert)
* **Fragment Shader**: [`world_warp.frag`](../../stuff/shaders/world_warp.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (generates fullscreen triangle in vertex shader from `gl_VertexIndex`; pushes 68-byte dummy buffer)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `44 bytes` (11 floats, offsets `68`–`111`):
    * `layout(offset = 68) float time;` (4 bytes)
    * `layout(offset = 72) float scale;` (4 bytes)
    * `layout(offset = 76) float scrWidth;` (4 bytes)
    * `layout(offset = 80) float scrHeight;` (4 bytes)
    * `layout(offset = 84) float offsetX;` (4 bytes)
    * `layout(offset = 88) float offsetY;` (4 bytes)
    * `layout(offset = 92) float pixelSize;` (4 bytes)
    * `layout(offset = 96) float refdefX;` (4 bytes)
    * `layout(offset = 100) float refdefY;` (4 bytes)
    * `layout(offset = 104) float refdefWidth;` (4 bytes)
    * `layout(offset = 108) float refdefHeight;` (4 bytes)

---

### 17. World Postprocess / Gamma Blit
* **Pipelines**: `vk_postprocessPipeline`
* **Vertex Shader**: [`postprocess.vert`](../../stuff/shaders/postprocess.vert)
* **Fragment Shader**: [`postprocess.frag`](../../stuff/shaders/postprocess.frag)
* **Vertex Constants**:
  * **Offset**: `0` | **Pipeline Layout Range**: `68 bytes`
  * **Shader Declared**: None (generates fullscreen triangle in vertex shader from `gl_VertexIndex`)
* **Fragment Constants**:
  * **Offset**: `68` | **Pipeline Layout Range**: `44 bytes`
  * **Shader Declared**: `24 bytes` (6 floats, offsets `68`–`91`):
    * `layout(offset = 68) float postprocess;` (4 bytes)
    * `layout(offset = 72) float gamma;` (4 bytes)
    * `layout(offset = 76) float scrWidth;` (4 bytes)
    * `layout(offset = 80) float scrHeight;` (4 bytes)
    * `layout(offset = 84) float offsetX;` (4 bytes)
    * `layout(offset = 88) float offsetY;` (4 bytes)

---

## 4. Master Summary Table

| Combination # | Vertex Shader | Fragment Shader | Vertex Constant Offset (Start) | Vertex Constant Declared Size | Fragment Constant Offset (Start) | Fragment Constant Declared Size | Pipeline Layout Allocations (V/F) |
|---|---|---|---|---|---|---|---|
| **1** | `basic.vert` | `basic.frag` | `0` | *None (UBO)* | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **2** | `basic_tinted.vert` | `basic.frag` | `0` | *None (UBO)* | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **3** | `particle.vert` | `basic.frag` | **`0`** | `64 B` (16 floats) | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **4** | `point_particle.vert` | `point_particle.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `24 B` (6 floats) | 68 B / 44 B |
| **5** | `basic_color_quad.vert` | `basic_color_quad.frag` | `0` | *None (UBO)* | **`72`** | `8 B` (2 floats) | 68 B / 44 B |
| **6** | `nullmodel.vert` | `basic_color_quad.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `8 B` (2 floats) | 68 B / 44 B |
| **7** | `model.vert` | `model.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `24 B` (6 floats) | 68 B / 44 B |
| **8** | `sprite.vert` | `basic.frag` | **`0`** | `68 B` (17 floats) | **`68`** | `28 B` (7 floats) | **80 B** / 44 B |
| **9** | `polygon.vert` | `basic.frag` | **`0`** | `64 B` (16 floats) | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **10** | `polygon_lmap.vert` | `polygon_lmap.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `24 B` (6 floats) | 68 B / 44 B |
| **11** | `polygon_warp.vert` | `basic.frag` | **`0`** | `64 B` (16 floats) | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **12** | `beam.vert` | `basic_color_quad.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `8 B` (2 floats) | 68 B / 44 B |
| **13** | `skybox.vert` | `basic.frag` | **`0`** | `64 B` (16 floats) | **`68`** | `28 B` (7 floats) | 68 B / 44 B |
| **14** | `d_light.vert` | `basic_color_quad.frag` | `0` | *None (UBO)* | **`72`** | `8 B` (2 floats) | 68 B / 44 B |
| **15** | `shadows.vert` | `basic_color_quad.frag` | **`0`** | `64 B` (16 floats) | **`72`** | `8 B` (2 floats) | 68 B / 44 B |
| **16** | `world_warp.vert` | `world_warp.frag` | `0` | *None (Index)* | **`68`** | `44 B` (11 floats) | 68 B / 44 B |
| **17** | `postprocess.vert` | `postprocess.frag` | `0` | *None (Index)* | **`68`** | `24 B` (6 floats) | 68 B / 44 B |
