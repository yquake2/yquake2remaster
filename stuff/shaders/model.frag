#version 450

layout(push_constant) uniform PostPushConstant
{
	// applied when the world is drawn straight into the swapchain image,
	// otherwise the postprocess pass does it
	layout(offset = 72) float postprocess;
	layout(offset = 76) float postGamma;
	layout(offset = 80) vec4 fogColor;
} pcPost;


layout(set = 0, binding = 0) uniform sampler2D sTexture;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in flat int textured;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	if(textured != 0)
		fragmentColor = texture(sTexture, texCoord) * clamp(color, 0.0, 1.0);
	else
		fragmentColor = color;

	if (pcPost.fogColor.a > 0.0)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float d = pcPost.fogColor.a * depth;
		float fogFactor = 1.0 - exp(-(d * d));
		fragmentColor.rgb = mix(fragmentColor.rgb, pcPost.fogColor.rgb, fogFactor);
	}

	if (pcPost.postprocess > 0.0)
	{
		fragmentColor.rgb = pow(fragmentColor.rgb * 1.5, vec3(pcPost.postGamma));
	}
}
