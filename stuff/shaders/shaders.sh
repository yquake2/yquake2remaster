#!/bin/sh

glslangValidator --variable-name basic_vert_spv -V basic.vert -o ../../src/client/refresh/vk/spirv/basic_vert.c
glslangValidator --variable-name basic_frag_spv -V basic.frag -o ../../src/client/refresh/vk/spirv/basic_frag.c
glslangValidator --variable-name basic_color_quad_vert_spv -V basic_color_quad.vert -o ../../src/client/refresh/vk/spirv/basic_color_quad_vert.c
glslangValidator --variable-name basic_color_quad_frag_spv -V basic_color_quad.frag -o ../../src/client/refresh/vk/spirv/basic_color_quad_frag.c
glslangValidator --variable-name model_vert_spv -V model.vert -o ../../src/client/refresh/vk/spirv/model_vert.c
glslangValidator --variable-name model_frag_spv -V model.frag -o ../../src/client/refresh/vk/spirv/model_frag.c
glslangValidator --variable-name nullmodel_vert_spv -V nullmodel.vert -o ../../src/client/refresh/vk/spirv/nullmodel_vert.c
glslangValidator --variable-name particle_vert_spv -V particle.vert -o ../../src/client/refresh/vk/spirv/particle_vert.c
glslangValidator --variable-name point_particle_vert_spv -V point_particle.vert -o ../../src/client/refresh/vk/spirv/point_particle_vert.c
glslangValidator --variable-name point_particle_frag_spv -V point_particle.frag -o ../../src/client/refresh/vk/spirv/point_particle_frag.c
glslangValidator --variable-name sprite_vert_spv -V sprite.vert -o ../../src/client/refresh/vk/spirv/sprite_vert.c
glslangValidator --variable-name beam_vert_spv -V beam.vert -o ../../src/client/refresh/vk/spirv/beam_vert.c
glslangValidator --variable-name skybox_vert_spv -V skybox.vert -o ../../src/client/refresh/vk/spirv/skybox_vert.c
glslangValidator --variable-name d_light_vert_spv -V d_light.vert -o ../../src/client/refresh/vk/spirv/d_light_vert.c
glslangValidator --variable-name polygon_vert_spv -V polygon.vert -o ../../src/client/refresh/vk/spirv/polygon_vert.c
glslangValidator --variable-name polygon_lmap_vert_spv -V polygon_lmap.vert -o ../../src/client/refresh/vk/spirv/polygon_lmap_vert.c
glslangValidator --variable-name polygon_lmap_frag_spv -V polygon_lmap.frag -o ../../src/client/refresh/vk/spirv/polygon_lmap_frag.c
glslangValidator --variable-name polygon_warp_vert_spv -V polygon_warp.vert -o ../../src/client/refresh/vk/spirv/polygon_warp_vert.c
glslangValidator --variable-name shadows_vert_spv -V shadows.vert -o ../../src/client/refresh/vk/spirv/shadows_vert.c
glslangValidator --variable-name postprocess_vert_spv -V postprocess.vert -o ../../src/client/refresh/vk/spirv/postprocess_vert.c
glslangValidator --variable-name postprocess_frag_spv -V postprocess.frag -o ../../src/client/refresh/vk/spirv/postprocess_frag.c
glslangValidator --variable-name world_warp_vert_spv -V world_warp.vert -o ../../src/client/refresh/vk/spirv/world_warp_vert.c
glslangValidator --variable-name world_warp_frag_spv -V world_warp.frag -o ../../src/client/refresh/vk/spirv/world_warp_frag.c
