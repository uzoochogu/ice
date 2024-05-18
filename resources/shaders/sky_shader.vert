#version 450

// vec4 for easy alignment
layout(set = 0, binding = 0) uniform CameraVectors {
	vec4 forwards;
	vec4 right;
	vec4 up;
} cameraData;

layout(location = 0) out vec3 forwards;

/*
**  Basically a rectangle using Vulkan coordinate system
**  -1,-1 and 1.0, 1.0 (for top-left and bottom-right) 
*/
const vec2 screen_corners[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0,  1.0),
	vec2( 1.0,  1.0),
	vec2( 1.0,  1.0),
	vec2( 1.0, -1.0),
	vec2(-1.0, -1.0)
);

void main() {
	// pick corner per vertex index
	vec2 pos = screen_corners[gl_VertexIndex];
	// out variable
	gl_Position = vec4(pos, 0.0, 1.0);
	// -ve accounts for Vulkan coordinate system
	forwards = normalize(cameraData.forwards + pos.x * cameraData.right - pos.y * cameraData.up).xyz;
}
