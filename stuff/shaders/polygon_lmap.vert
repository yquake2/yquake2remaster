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
    uint numDynLights;
} ubo;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 texCoordLmap;
layout(location = 2) out float viewLightmaps;
layout(location = 3) out vec3 worldCoord;
layout(location = 4) out vec3 worldNormal;
layout(location = 5) flat out uint lightFlags;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec4 world = ubo.model * vec4(inVertex, 1.0);

    gl_Position = pc.vpMatrix * world;
    texCoord = inTexCoord;
    texCoordLmap = inTexCoordLmap;
    viewLightmaps = ubo.viewLightmaps;
    worldCoord = world.xyz;
    worldNormal = normalize((ubo.model * vec4(inNormal, 0.0)).xyz);
    lightFlags = uint(inFlags);
}
