#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inTexCoordLmap;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in int inFlags;

layout(push_constant) uniform PushConstant
{
    mat4 vpMatrix;
} pc;

layout(set = 1, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    float viewLightmaps;
} ubo;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 texCoordLmap;
layout(location = 2) out float viewLightmaps;
layout(location = 3) out float depth;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 worldPos = ubo.model * vec4(inVertex, 1.0);
    gl_Position = pc.vpMatrix * worldPos;
    texCoord = inTexCoord;
    texCoordLmap = inTexCoordLmap;
    viewLightmaps = ubo.viewLightmaps;

    // Calculate depth for fog
    depth = length(worldPos.xyz);
}
