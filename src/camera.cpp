#include "camera.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace ice {

Camera::Camera(CameraDimensions dim, glm::vec3 position)
    : camera_matrix{},
      position(position),
      DEFAULT_POSITION(position),
      width(dim.width),
      height(dim.height) {}

CameraMatrices Camera::get_camera_matrix() const { return camera_matrix; }

CameraVectors Camera::get_camera_vector() const { return camera_vector; }

void Camera::update_matrices(float FOV_deg, float near_plane, float far_plane) {
  camera_matrix.view = glm::lookAt(position, position + orientation, up);
  camera_matrix.projection =
      glm::perspective(glm::radians(FOV_deg),
                       static_cast<float>(width) / static_cast<float>(height),
                       near_plane, far_plane);
  camera_matrix.projection[1][1] *= -1;  // correct to Vulkan coordinate system
  camera_matrix.view_projection = camera_matrix.projection * camera_matrix.view;

  // Update camera_vector
  camera_vector.forwards = glm::vec4(glm::normalize(orientation), 0.0f);
  camera_vector.right =
      glm::vec4(glm::normalize(glm::cross(orientation, up)), 0.0f);
  // maintain orthogonality
  camera_vector.up =
      glm::vec4(glm::normalize(glm::cross(glm::vec3(camera_vector.right),
                                          glm::vec3(camera_vector.forwards))),
                0.0f);
  camera_vector.tanHalfFovY =
      -1.0f /
      camera_matrix.projection[1][1];  // use original y axis, tan(fovY * 0.5);
  camera_vector.tanHalfFovX =
      1.0f / camera_matrix.projection[0][0];  // tanHalfFovY * aspectRatio;
}

void Camera::inputs(IceWindow *ice_window) {
  GLFWwindow *window = ice_window->get_window();
  // Check if ImGui wants to capture the mouse
  if (ImGui::GetIO().WantCaptureMouse) {
    camera_active = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    return;
  }

  // Handles key inputs
  if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
    // Resets to origin
    position = DEFAULT_POSITION;
    orientation = glm::vec3(0.0f, 0.0f, 1.0f);  // Forward along +Z
    up = glm::vec3(0.0f, 1.0f, 0.0f);           // Up along -Y
    speed = default_speed = 0.005f;
    sensitivity = 5.0f;
  }
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
    // increase speed and sensitivity
    default_speed += 0.01f;
    sensitivity += 0.50f;
  }
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
    // decrease speed and sensitivity
    default_speed = std::max(default_speed - 0.01f, 0.0f);
    sensitivity = std::max(sensitivity - 0.50f, 0.0f);
  }
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    // reset speed and sensitivity
    speed = default_speed = 0.005f;
    sensitivity = 5.0f;
  }
#ifndef NDEBUG
  if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS) {
    glfwWaitEvents();
    if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_RELEASE) {
      // Dump values to console
      std::cout << std::format(
                       "\n\nCamera Data:\n"
                       "Position:\n{}\n"
                       "Orientation:\n{}\n"
                       "Speed:\n{}\n"
                       "Sensitivity:\n{}\n"
                       "++++++++++++++\n"
                       "Camera Matrix:\n"
                       "++++++++++++++\n"
                       "view:\n{}\n"
                       "projection:\n{}\n"
                       "view-projection:\n{}\n",
                       glm::to_string(position), glm::to_string(orientation),
                       speed, sensitivity, glm::to_string(camera_matrix.view),
                       glm::to_string(camera_matrix.projection),
                       glm::to_string(camera_matrix.projection))
                << std::endl;
      // Camera vectors
      std::cout << std::format(
                       "Camera Vectors:\n"
                       "  Forward: {}\n"
                       "  Right: {}\n"
                       "  Up: {}\n"
                       "  Orthogonality check:\n"
                       "    Forward·Right: {} (should be ~0)\n"
                       "    Forward·Up: {} (should be ~0)\n"
                       "    Right·Up: {} (should be ~0)\n",
                       glm::to_string(camera_vector.forwards),
                       glm::to_string(camera_vector.right),
                       glm::to_string(camera_vector.up),
                       glm::dot(glm::vec3(camera_vector.forwards),
                                glm::vec3(camera_vector.right)),
                       glm::dot(glm::vec3(camera_vector.forwards),
                                glm::vec3(camera_vector.up)),
                       glm::dot(glm::vec3(camera_vector.right),
                                glm::vec3(camera_vector.up)))
                << std::endl;
    }
  }
#endif
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    position += speed * orientation;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
    position += speed * -glm::normalize(glm::cross(orientation, up));
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    position += speed * -orientation;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    position += speed * glm::normalize(glm::cross(orientation, up));
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    position += speed * up;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    position += speed * -up;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    speed = 0.02f;
  } else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
    speed = default_speed;
  }

  // Handles mouse inputs
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    if (!camera_active) {
      camera_active = true;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    // Prevents camera from jumping on the first click
    if (first_click) {
      glfwSetCursorPos(window, static_cast<float>(width) / 2.0f,
                       static_cast<float>(height) / 2.0f);  // middle of screen
      first_click = false;
    }

    double mouse_x{0.0}, mouse_y{0.0};
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    // Normalizes and shifts the coordinates of the cursor such that they begin
    // in the middle of the screen and then "transforms" them into degrees
    const float rot_x =
        sensitivity *
        static_cast<float>(mouse_y - (static_cast<float>(height) / 2)) /
        static_cast<float>(height);
    const float rot_y =
        sensitivity *
        static_cast<float>(mouse_x - (static_cast<float>(width) / 2)) /
        static_cast<float>(width);

    // Calculates upcoming vertical change in the orientation
    orientation = glm::rotate(orientation, glm::radians(-rot_x),
                              glm::normalize(glm::cross(orientation, up)));

    // Rotates the orientation left and right
    orientation = glm::rotate(orientation, glm::radians(-rot_y), up);

    glfwSetCursorPos(window, static_cast<float>(width) / 2.0f,
                     static_cast<float>(height) / 2.0f);
  } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
             GLFW_RELEASE) {
    if (camera_active) {
      camera_active = false;
      glfwSetInputMode(window, GLFW_CURSOR,
                       GLFW_CURSOR_NORMAL);  // not looking anymore
    }
    // Makes sure the next time the camera looks around it doesn't jump
    first_click = true;
  }
}

}  // namespace ice
