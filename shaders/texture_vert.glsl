#version 450 core
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform UBO {
	mat4 modelViewProj;
	mat4 modelView;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 3) in int in_texIndex;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texcoord;
layout(location = 2) out vec3 out_position;
layout(location = 3) out flat int out_texIndex;

void main() {
	
	// Define precision, e.g., for two decimal places
float p = 100.0; // 100.0 for two decimal places, 10.0 for one decimal place, etc.

vec4 pos = ubo.modelViewProj * vec4(in_position, 1.0);

// Round each component of `gl_Position` to the nearest specified precision
gl_Position = vec4(
    round(pos.x * p) / p,
    round(pos.y * p) / p,
    round(pos.z * p) / p,
    pos.w
);
	//gl_Position = ubo.modelViewProj * vec4(in_position, 1.0);
	out_texcoord = in_texcoord;
	out_normal = mat3(transpose(inverse(ubo.modelView))) * in_normal;
	out_position = (ubo.modelView * vec4(in_position, 1.0)).xyz;
	out_texIndex = in_texIndex;
}