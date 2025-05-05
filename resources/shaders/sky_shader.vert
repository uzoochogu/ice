#version 450

// Camera vectors for view direction calculation
layout(set = 0, binding = 0) uniform CameraVectors {
  vec4 forwards;
  vec4 right;
  vec4 up;
  float tanHalfFovY;
  float tanHalfFovX;
}
cameraData;

layout(location = 0) out vec3 forwards;

// Screen-space quad vertices
const vec2 screen_corners[6] =
    vec2[](vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0),
           vec2(1.0, 1.0), vec2(1.0, -1.0));

void main() {
  // Use the screen-space quad directly (no transformations)
  vec2 pos = screen_corners[gl_VertexIndex];
  gl_Position = vec4(pos, 0.0, 1.0);

  // Calculate ray direction with proper FOV scaling
  vec3 rayDir;
  rayDir.x = pos.x * cameraData.tanHalfFovX;
  rayDir.y = pos.y * cameraData.tanHalfFovY;
  rayDir.z = 1.0;

  // Transform ray to world space
  mat3 viewToWorld = mat3(
      cameraData.right.xyz,
      -cameraData.up
           .xyz,  // negated it to maintain the right-handed coordinate system??
      cameraData.forwards.xyz);

  forwards = normalize(viewToWorld * rayDir);

  // vector method (Equivalent to the above)
  // // Scale the position by the tangent of half the FOV
  // vec2 scaledPos = vec2(pos.x * tanHalfFovX, pos.y * tanHalfFovY);

  // // Calculate the view direction
  // forwards = normalize(
  //     cameraData.forwards.xyz +
  //     scaledPos.x * cameraData.right.xyz -
  //     scaledPos.y * cameraData.up.xyz
  // );
}
