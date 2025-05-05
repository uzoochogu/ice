#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(set = 1, binding = 0) uniform sampler2D material;

layout(location = 0) out vec4 outColor;

const vec4 sunColor = vec4(1.0, 1.0, 1.0, 1.0);
/* Sun is located Right (+x), Up (-y in Vulkan) and forward out of screen(+z)
 */
const vec3 sunDirection = normalize(vec3(1.0, -1.0, 1.0));

void main() {
  /* We want the surfaces pointing back at us to be illuminated */
  outColor = sunColor * max(0.0, dot(fragNormal, -sunDirection)) *
             vec4(fragColor, 1.0) * texture(material, fragTexCoord);
}