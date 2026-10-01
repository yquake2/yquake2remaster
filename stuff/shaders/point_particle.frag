#version 450

layout(push_constant) uniform PostPushConstant
{
	// applied when the world is drawn straight into the swapchain image,
	// otherwise the postprocess pass does it
	layout(offset = 72) float postprocess;
	layout(offset = 76) float postGamma;
	layout(offset = 80) vec4 fogColor;
} pcPost;


layout(location = 0) in vec4 color;

layout(location = 0) out vec4 fragmentColor;

void main()
{
	vec2 cxy = 2.0 * gl_PointCoord - 1.0;

	if(dot(cxy, cxy) > 1.0)
		discard;

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
