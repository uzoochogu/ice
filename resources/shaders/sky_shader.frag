#version 450

layout(location = 0) in vec3 forwards;

layout(set = 1, binding = 0) uniform samplerCube material;

layout(location = 0) out vec4 outColor;

void main() {
	// using cube sampler read using the forwards coordinate/direction vec
	outColor = texture(material, forwards);
}
