#version 450

layout(set = 0, binding = 0) uniform sampler2D sTexture;
layout(set = 2, binding = 0) uniform sampler2D sLightmap;

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

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 texCoordLmap;
layout(location = 2) in float viewLightmaps;
layout(location = 3) in float depth;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 color = texture(sTexture, texCoord);
    vec4 light = texture(sLightmap, texCoordLmap);
    fragmentColor = (1.0 - viewLightmaps) * color * light + viewLightmaps * light;

    // Apply fog if enabled
    if (fog.fogColor.w > 0.0)
    {
        float d = fog.fogColor.w * depth;
        float fogFactor = 1.0 - exp(-(d * d));
        fragmentColor.rgb = mix(fragmentColor.rgb, fog.fogColor.rgb, fogFactor);
    }
}
