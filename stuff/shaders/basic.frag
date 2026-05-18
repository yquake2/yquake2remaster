#version 450

layout(push_constant) uniform PushConstant
{
	// vertex shader has 'mat4 vpMatrix;' at begin.
	layout(offset = 68) float gamma;
} pc;

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(set = 2, binding = 0) uniform FogParameters
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
layout(location = 1) in vec4 color;
layout(location = 2) in float aTreshold;
layout(location = 3) in float depth;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	fragmentColor = texture(sTexture, texCoord) * color;
	if(fragmentColor.a < aTreshold)
		discard;

	fragmentColor = vec4(pow(fragmentColor.rgb, vec3(pc.gamma)), fragmentColor.a);

	// Apply global fog if enabled (density > 0)
	if (fog.fogColor.w > 0.0)
	{
	    float d = fog.fogColor.w * depth;
	    float fogFactor = 1.0 - exp(-(d * d)); // quadratic exponential falloff
	    fragmentColor.rgb = mix(fragmentColor.rgb, fog.fogColor.rgb, fogFactor);
	}
}
