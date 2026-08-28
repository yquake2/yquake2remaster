#version 450

layout(push_constant) uniform PostPushConstant
{
	// applied when the world is drawn straight into the swapchain image,
	// otherwise the postprocess pass does it
	layout(offset = 112) float postprocess;
	layout(offset = 116) float postGamma;
} pcPost;


layout(location = 0) in vec4 color;

layout(location = 0) out vec4 fragmentColor;

void main()
{
    fragmentColor = color;
	if (pcPost.postprocess > 0.0)
	{
		fragmentColor.rgb = pow(fragmentColor.rgb * 1.5, vec3(pcPost.postGamma));
	}
}
