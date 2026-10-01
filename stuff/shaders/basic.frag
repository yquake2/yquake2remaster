#version 450

layout(push_constant) uniform PushConstant
{
	// vertex shader has 'mat4 vpMatrix;' at begin.
	layout(offset = 68) float gamma;
	layout(offset = 72) float postprocess;
	layout(offset = 76) float postGamma;
	layout(offset = 80) vec4 fogColor;
} pc;

layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 color;
layout(location = 2) in float aTreshold;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	fragmentColor = texture(sTexture, texCoord) * color;
	if(fragmentColor.a < aTreshold)
		discard;

	fragmentColor = vec4(pow(fragmentColor.rgb, vec3(pc.gamma)), fragmentColor.a);
	if (pc.fogColor.a > 0.0)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float d = pc.fogColor.a * depth;
		float fogFactor = 1.0 - exp(-(d * d));
		fragmentColor.rgb = mix(fragmentColor.rgb, pc.fogColor.rgb, fogFactor);
	}

	if (pc.postprocess > 0.0)
	{
		fragmentColor.rgb = pow(fragmentColor.rgb * 1.5, vec3(pc.postGamma));
	}
}
