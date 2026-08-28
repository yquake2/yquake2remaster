#version 450

// keep in sync with MAX_DLIGHTS
#define MAX_DYN_LIGHTS 32

layout(set = 0, binding = 0) uniform sampler2D sTexture;
layout(set = 2, binding = 0) uniform sampler2D sLightmap;

layout(set = 1, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    float viewLightmaps;
    uint numDynLights;
} ubo;

struct DynLight
{
    vec4 origin; // world position, w unused
    vec4 color;  // RGB + intensity in .a
};

layout(set = 3, binding = 0) uniform DynLightBufferObject
{
    DynLight dynLights[MAX_DYN_LIGHTS];
} lights;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 texCoordLmap;
layout(location = 2) in float viewLightmaps;
layout(location = 3) in vec3 worldCoord;
layout(location = 4) in vec3 worldNormal;
layout(location = 5) flat in uint lightFlags;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    vec4 color = texture(sTexture, texCoord);
    vec4 light = texture(sLightmap, texCoordLmap);

    if (lightFlags != 0u)
    {
        for (uint i = 0u; i < ubo.numDynLights; ++i)
        {
            // dynamic light i does not reach this surface, skip it
            if ((lightFlags & (1u << i)) == 0u)  continue;

            float intens = lights.dynLights[i].color.a;

            vec3 lightToPos = lights.dynLights[i].origin.xyz - worldCoord;
            float distLightToPos = length(lightToPos);
            float fact = max(0.0, intens - distLightToPos - 52.0);

            // move the light source a bit further above the surface
            // => helps if the lightsource is so close to the surface (e.g. grenades, rockets)
            //    that the dot product below would return 0
            // (light sources that are below the surface are filtered out by lightFlags)
            lightToPos += worldNormal * 32.0;

            // also factor in angle between light and point on surface
            fact *= max(0.0, dot(worldNormal, normalize(lightToPos)));

            light.rgb += lights.dynLights[i].color.rgb * fact * (1.0 / 256.0);
        }
    }

    fragmentColor = (1.0 - viewLightmaps) * color * light + viewLightmaps * light;
}
